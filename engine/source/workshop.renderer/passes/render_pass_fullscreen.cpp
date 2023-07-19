// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/passes/render_pass_fullscreen.h"
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

void render_pass_fullscreen::generate(renderer& renderer, generated_state& state_output, render_view& view)
{
    // Grab fullscreen buffers for the render pass.
    ri_data_layout layout;
    ri_buffer* vertex_buffer = nullptr;
    ri_buffer* index_buffer = nullptr;
    if (technique)
    {
        layout = technique->pipeline->get_create_params().vertex_layout;
        renderer.get_fullscreen_buffers(layout, vertex_buffer, index_buffer);
    }

    // Grab and update the vertex info buffer.
    ri_param_block* vertex_info_param_block = nullptr;
    
    if (technique)
    {
        vertex_info_param_block = view.get_resource_cache().find_or_create_param_block(get_cache_key(view), "vertex_info");
        vertex_info_param_block->set("vertex_buffer", *vertex_buffer);
        vertex_info_param_block->set("vertex_buffer_offset", 0u);
        vertex_info_param_block->clear_buffer("instance_buffer");
        param_blocks.push_back(vertex_info_param_block);
    }

    // Create the command list.
    ri_command_list& list = renderer.get_render_interface().get_graphics_queue().alloc_command_list();
    list.open();
    {
        profile_gpu_marker(list, profile_colors::gpu_pass, "%s", name.c_str());

        // Transition targets to the relevant state.
        for (ri_texture_view texture : output.color_targets)
        {
            list.barrier(*texture.texture, ri_resource_state::initial, ri_resource_state::render_target);

            if (clear_color_outputs)
            {
                list.clear(texture, color::black);
            }
        }
        if (output.depth_target.texture != nullptr)
        {
            list.barrier(*output.depth_target.texture, ri_resource_state::initial, ri_resource_state::depth_write);

            if (clear_depth_outputs)
            {
                list.clear_depth(output.depth_target, 1.0f, 0);
            }
        }

        // Technique can be null if we are just using this pass as an excuse to clear the depth depth/color targets.
        if (technique)
        {
            list.set_pipeline(*technique->pipeline.get());
            list.set_render_targets(output.color_targets, output.depth_target);
            list.set_param_blocks(bind_param_blocks(view.get_resource_cache()));
            list.set_viewport(view.get_viewport());
            list.set_scissor(view.get_viewport());
            list.set_primitive_topology(ri_primitive::triangle_list);
            list.set_index_buffer(*index_buffer);
            list.draw(6, 1);
        }

        // Transition targets back to initial state.
        for (ri_texture_view texture : output.color_targets)
        {
            list.barrier(*texture.texture, ri_resource_state::render_target, ri_resource_state::initial);
        }
        if (output.depth_target.texture != nullptr)
        {
            list.barrier(*output.depth_target.texture, ri_resource_state::depth_write, ri_resource_state::initial);
        }
    }
    list.close();

    state_output.graphics_command_lists.push_back(&list);
}

}; // namespace ws
