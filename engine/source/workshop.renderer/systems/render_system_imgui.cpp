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
#include "workshop.renderer/passes/render_pass_imgui.h"
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

void render_system_imgui::build_post_graph(render_graph& graph, const render_world_state& state)
{
    if (m_draw_vertices.empty() || m_draw_indices.empty())
    {
        return;
    }

    // TODO: When we have multi viewport support add a pass for each swapchain.

    m_model_info_param_block = m_renderer.get_param_block_manager().create_param_block("model_info");
    m_model_info_param_block->set("index_size", (int)m_index_buffer->get_element_size());
    m_model_info_param_block->set("position_buffer", *m_position_buffer);
    m_model_info_param_block->set("uv0_buffer", *m_uv0_buffer);
    m_model_info_param_block->set("color0_buffer", *m_color0_buffer);

    size_t model_info_table_index;
    size_t model_info_table_offset;
    m_model_info_param_block->get_table(model_info_table_index, model_info_table_offset);

    m_vertex_info_param_block = m_renderer.get_param_block_manager().create_param_block("vertex_info");
    m_vertex_info_param_block->set("model_info_table", (uint32_t)model_info_table_index);
    m_vertex_info_param_block->set("model_info_offset", (uint32_t)model_info_table_offset);
    m_vertex_info_param_block->set("material_info_table", 0);
    m_vertex_info_param_block->set("material_info_offset", 0);
    m_vertex_info_param_block->clear_buffer("instance_buffer");

    m_imgui_param_blocks.clear();
    if (m_imgui_param_blocks.size() < m_draw_commands.size())
    {
        m_imgui_param_blocks.resize(m_draw_commands.size());
    }

    size_t index = 0;
    for (render_system_imgui::draw_command& cmd : m_draw_commands)
    {
        std::unique_ptr<ri_param_block>& param_block = m_imgui_param_blocks[index];
        if (!param_block)
        {
            param_block = m_renderer.get_param_block_manager().create_param_block("imgui_params");
        }

        cmd.param_block = param_block.get();
        index++;
    }

    std::unique_ptr<render_pass_imgui> pass = std::make_unique<render_pass_imgui>();
    pass->name = "imgui";
    pass->system = this;
    pass->technique = m_renderer.get_effect_manager().get_technique("render_imgui", {});
    pass->output = m_renderer.get_swapchain_output();
    pass->default_texture = m_default_texture.get();
    pass->position_buffer = m_position_buffer.get();
    pass->uv0_buffer = m_uv0_buffer.get();
    pass->color0_buffer = m_color0_buffer.get();
    pass->index_buffer = m_index_buffer.get();
    pass->draw_commands = m_draw_commands;
    pass->param_blocks.push_back(m_vertex_info_param_block.get());
    graph.add_node(std::move(pass));
}

void render_system_imgui::step(const render_world_state& state)
{
    ri_interface& ri = m_renderer.get_render_interface();

    if (m_draw_vertices.empty() || m_draw_indices.empty())
    {
        return;
    }

    //db_log(renderer, "==== IMGUI: %zi ====", m_draw_vertices.size());

    // TODO: Should we be using a layout factory for all this?

    // Make sure vertex buffer has enough space.
    if (m_position_buffer == nullptr || m_position_buffer->get_element_count() < m_draw_vertices.size())
    {
        ri_buffer::create_params params;
        params.element_count = m_draw_vertices.size();
        params.element_size = sizeof(vector3);
        params.usage = ri_buffer_usage::vertex_buffer;

        m_position_buffer = ri.create_buffer(params, "ImGui Vertex Buffer [position]");
    }

    if (m_uv0_buffer == nullptr || m_uv0_buffer->get_element_count() < m_draw_vertices.size())
    {
        ri_buffer::create_params params;
        params.element_count = m_draw_vertices.size();
        params.element_size = sizeof(vector2);
        params.usage = ri_buffer_usage::vertex_buffer;

        m_uv0_buffer = ri.create_buffer(params, "ImGui Vertex Buffer [uv0]");
    }

    if (m_color0_buffer == nullptr || m_color0_buffer->get_element_count() < m_draw_vertices.size())
    {
        ri_buffer::create_params params;
        params.element_count = m_draw_vertices.size();
        params.element_size = sizeof(vector4);
        params.usage = ri_buffer_usage::vertex_buffer;

        m_color0_buffer = ri.create_buffer(params, "ImGui Vertex Buffer [color0]");
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
    vector3* position_ptr = reinterpret_cast<vector3*>(m_position_buffer->map(0, m_draw_vertices.size() * sizeof(vector3)));
    for (size_t i = 0; i < m_draw_vertices.size(); i++)
    {
        position_ptr[i] = vector3(m_draw_vertices[i].position.x, m_draw_vertices[i].position.y, 0.0f);
    }
    m_position_buffer->unmap(position_ptr);

    vector2* uv0_ptr = reinterpret_cast<vector2*>(m_uv0_buffer->map(0, m_draw_vertices.size() * sizeof(vector2)));
    for (size_t i = 0; i < m_draw_vertices.size(); i++)
    {
        uv0_ptr[i] = m_draw_vertices[i].uv;
    }
    m_uv0_buffer->unmap(uv0_ptr);

    vector4* color0_ptr = reinterpret_cast<vector4*>(m_color0_buffer->map(0, m_draw_vertices.size() * sizeof(vector4)));
    for (size_t i = 0; i < m_draw_vertices.size(); i++)
    {
        color0_ptr[i] = m_draw_vertices[i].color;
    }
    m_color0_buffer->unmap(color0_ptr);
    
    // Update index buffer.
    uint16_t* index_ptr = reinterpret_cast<uint16_t*>(m_index_buffer->map(0, m_draw_indices.size() * sizeof(uint16_t)));
    for (size_t i = 0; i < m_draw_indices.size(); i++)
    {
        index_ptr[i] = m_draw_indices[i];
    }
    m_index_buffer->unmap(index_ptr);
}

void render_system_imgui::set_default_texture(std::unique_ptr<ri_texture> texture)
{
    m_default_texture = std::move(texture);
}

void render_system_imgui::update_draw_data(const std::vector<draw_command>& commands, const std::vector<vertex>& vertices, const std::vector<uint16_t>& indices)
{
    m_draw_commands = commands;
    m_draw_vertices = vertices;
    m_draw_indices = indices;
}

}; // namespace ws
