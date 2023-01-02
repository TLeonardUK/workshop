// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_pass_fullscreen.h"
#include "workshop.renderer/render_world_state.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/common_types.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_command_queue.h"
#include "workshop.render_interface/ri_command_list.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface/ri_layout_factory.h"

namespace ws {

result<void> render_pass_fullscreen::create_resources(renderer& renderer)
{
    ri_data_layout layout = technique->pipeline->get_create_params().vertex_layout;

    // Generate vertex/index buffers for rendering a rect across the entire screen.
    std::unique_ptr<ri_layout_factory> factory = renderer.get_render_interface().create_layout_factory(layout);
    factory->add("position", { vector2(-1.0f, -1.0f), vector2( 1.0f, -1.0f), vector2(-1.0f,  1.0f), vector2( 1.0f,  1.0f) });    
    factory->add("uv",       { vector2( 0.0f,  0.0f), vector2( 1.0f,  0.0f), vector2( 0.0f,  1.0f), vector2( 1.0f,  1.0f) });

    m_vertex_buffer = factory->create_vertex_buffer(string_format("%s - Vertex buffer", name.c_str()).c_str());
    m_index_buffer  = factory->create_index_buffer(string_format("%s - Index buffer", name.c_str()).c_str(), std::vector<uint16_t> { 2, 1, 0, 3, 1, 2 });

    // Create the main vertex info param block.
    m_vertex_info_param_block = renderer.get_param_block_manager().create_param_block("vertex_info");
    m_vertex_info_param_block->set("vertex_buffer", *m_vertex_buffer);
    m_vertex_info_param_block->set("vertex_buffer_offset", 0u);
    param_blocks.push_back(m_vertex_info_param_block.get());

    // Validate the parameters we've been passed.
    if (result<void> ret = validate_parameters(); !ret)
    {
        return ret;
    }

    return true;
}

result<void> render_pass_fullscreen::destroy_resources(renderer& renderer)
{
    m_vertex_info_param_block = nullptr;

    m_index_buffer = nullptr;
    m_vertex_buffer = nullptr;

    return true;
}

void render_pass_fullscreen::generate(renderer& renderer, generated_state& state_output, render_view& view)
{
    ri_command_list& list = renderer.get_render_interface().get_graphics_queue().alloc_command_list();
    list.open();
    {
        profile_gpu_marker(list, profile_colors::gpu_pass, "%s", name.c_str());

        // Transition targets to the relevant state.
        for (ri_texture* texture : output.color_targets)
        {
            list.barrier(*texture, ri_resource_state::initial, ri_resource_state::render_target);
        }
        if (output.depth_target)
        {
            list.barrier(*output.depth_target, ri_resource_state::initial, ri_resource_state::depth_write);
        }

        list.set_pipeline(*technique->pipeline.get());
        list.set_render_targets(output.color_targets, output.depth_target);
        list.set_param_blocks(param_blocks);
        list.set_viewport(view.viewport);
        list.set_scissor(view.viewport);
        list.set_primitive_topology(ri_primitive::triangle_list);
        list.set_index_buffer(*m_index_buffer);
        list.draw(6, 1);

        // Transition targets back to initial state.
        for (ri_texture* texture : output.color_targets)
        {
            list.barrier(*texture, ri_resource_state::render_target, ri_resource_state::initial);
        }
        if (output.depth_target)
        {
            list.barrier(*output.depth_target, ri_resource_state::depth_write, ri_resource_state::initial);
        }
    }
    list.close();

    state_output.graphics_command_lists.push_back(&list);
}

}; // namespace ws
