// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/passes/render_pass_clear.h"
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

void render_pass_clear::generate(renderer& renderer, generated_state& state_output, render_view* view)
{
    // Create the command list.
    ri_command_list& list = renderer.get_render_interface().get_graphics_queue().alloc_command_list();
    list.open();
    {
        profile_gpu_marker(list, profile_colors::gpu_pass, "%s", name.c_str());

        // Transition targets to the relevant state.
        for (ri_texture_view texture : output.color_targets)
        {
            list.barrier(*texture.texture, ri_resource_state::initial, ri_resource_state::render_target);
            list.clear(texture, texture.texture->get_optimal_clear_color());
        }
        if (output.depth_target.texture != nullptr)
        {
            list.barrier(*output.depth_target.texture, ri_resource_state::initial, ri_resource_state::depth_write);
            list.clear_depth(output.depth_target, 
                output.depth_target.texture->get_optimal_clear_depth(), 
                output.depth_target.texture->get_optimal_clear_stencil());
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
