// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_imgui.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"
#include "workshop.renderer/passes/render_pass_callback.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_command_queue.h"
#include "workshop.render_interface/ri_command_list.h"

#include "workshop.core/math/matrix4.h"
#include "workshop.core/perf/profile.h"

#include "workshop.render_interface/ri_interface.h"

namespace ws {

render_system_imgui::render_system_imgui(renderer& render)
    : render_system(render, "imgui")
{
}

void render_system_imgui::register_init(init_list& list)
{
}

void render_system_imgui::create_graph(render_graph& graph)
{
    std::unique_ptr<render_pass_callback> pass = std::make_unique<render_pass_callback>();
    pass->callback = std::bind(&render_system_imgui::generate, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    m_render_pass = pass.get();

    graph.add_node(std::move(pass), render_view_flags::normal);
}

void render_system_imgui::step(const render_world_state& state)
{
    ri_interface& ri = m_renderer.get_render_interface();

    if (m_draw_vertices.empty() || m_draw_indices.empty())
    {
        return;
    }

    // TODO: Should we be using a layout factory for all this?

    // Make sure vertex buffer has enough space.
    if (m_vertex_buffer == nullptr || m_vertex_buffer->get_element_count() < m_draw_vertices.size())
    {
        ri_buffer::create_params params;
        params.element_count = m_draw_vertices.size();
        params.element_size = sizeof(vertex);
        params.usage = ri_buffer_usage::vertex_buffer;

        m_vertex_buffer = ri.create_buffer(params, "ImGui Vertex Buffer");
    }

    // Make sure index buffer has enough space.
    if (m_index_buffer == nullptr || m_index_buffer->get_element_count() < m_draw_indices.size())
    {
        ri_buffer::create_params params;
        params.element_count = m_draw_indices.size();
        params.element_size = sizeof(uint16_t);
        params.usage = ri_buffer_usage::index_buffer;

        m_index_buffer = ri.create_buffer(params, "ImGui Index Buffer");
    }

    // Update vertex buffer.
    vertex* vertex_ptr = reinterpret_cast<vertex*>(m_vertex_buffer->map(0, m_draw_vertices.size() * sizeof(vertex)));
    for (size_t i = 0; i < m_draw_vertices.size(); i++)
    {
        vertex_ptr[i] = m_draw_vertices[i];
    }
    m_vertex_buffer->unmap(vertex_ptr);
    
    // Update index buffer.
    uint16_t* index_ptr = reinterpret_cast<uint16_t*>(m_index_buffer->map(0, m_draw_indices.size() * sizeof(uint16_t)));
    for (size_t i = 0; i < m_draw_indices.size(); i++)
    {
        index_ptr[i] = m_draw_indices[i];
    }
    m_index_buffer->unmap(index_ptr);
}

render_system_imgui::texture_id render_system_imgui::register_texture(std::unique_ptr<ri_texture> texture, bool default_texture)
{
    texture_id id = m_next_texture_id++;
    m_textures[id] = std::move(texture);

    if (default_texture)
    {
        m_default_texture = id;
    }

    return id;
}

void render_system_imgui::unregister_texture(texture_id id)
{
    auto iter = m_textures.find(id);
    if (iter != m_textures.end())
    {
        m_textures.erase(iter);
    }
}

void render_system_imgui::update_draw_data(const std::vector<draw_command>& commands, const std::vector<vertex>& vertices, const std::vector<uint16_t>& indices)
{
    m_draw_commands = commands;
    m_draw_vertices = vertices;
    m_draw_indices = indices;
}

void render_system_imgui::generate(renderer& renderer, render_pass::generated_state& state_output, render_view& view)
{
    if (m_draw_vertices.empty() || m_draw_indices.empty())
    {
        return;
    }

    ri_texture* default_texture = m_textures[m_default_texture].get();

    render_output output = m_renderer.get_swapchain_output();
    render_effect::technique* technique = m_renderer.get_effect_manager().get_technique("render_imgui", {});

    // TODO: Cache all these param blocks.

    std::unique_ptr<ri_param_block> vertex_info_param_block = renderer.get_param_block_manager().create_param_block("vertex_info");
    vertex_info_param_block->set("vertex_buffer", *m_vertex_buffer);
    vertex_info_param_block->set("vertex_buffer_offset", 0u);
    vertex_info_param_block->clear_buffer("instance_buffer");

    ri_command_list& list = renderer.get_render_interface().get_graphics_queue().alloc_command_list();
    list.open();
    {
        profile_gpu_marker(list, profile_colors::gpu_pass, "imgui");

        list.barrier(*output.color_targets[0], ri_resource_state::initial, ri_resource_state::render_target);
        list.set_pipeline(*technique->pipeline.get());
        list.set_render_targets(output.color_targets, nullptr);
        list.set_viewport(view.get_viewport());
        list.set_scissor(view.get_viewport());
        list.set_primitive_topology(ri_primitive::triangle_list);
        list.set_index_buffer(*m_index_buffer);

        for (draw_command& cmd : m_draw_commands)
        {
            std::unique_ptr<ri_param_block> imgui_params = renderer.get_param_block_manager().create_param_block("imgui_params");
            imgui_params->set("color_texture", *default_texture);
            imgui_params->set("color_sampler", *m_renderer.get_default_sampler(default_sampler_type::color));
            imgui_params->set("projection_matrix", matrix4::orthographic(
                cmd.display_pos.x,
                cmd.display_pos.x + cmd.display_size.x,
                cmd.display_pos.y,
                cmd.display_pos.y + cmd.display_size.y,
                0.0f,
                1.0f
            ));
            
            list.set_param_blocks({ vertex_info_param_block.get(), imgui_params.get() });
            list.set_scissor(recti((int)cmd.scissor.x, (int)cmd.scissor.y, (int)cmd.scissor.width, (int)cmd.scissor.height));
            list.draw(cmd.count, 1, cmd.offset);
        }

        list.barrier(*output.color_targets[0], ri_resource_state::render_target, ri_resource_state::initial);
        
    }
    list.close();

    state_output.graphics_command_lists.push_back(&list);
}

}; // namespace ws
