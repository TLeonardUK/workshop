// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/passes/render_pass_raytracing.h"
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

void render_pass_raytracing::generate(renderer& renderer, generated_state& state_output, render_view* view)
{
    // Create the command list.
    ri_command_list& list = renderer.get_render_interface().get_graphics_queue().alloc_command_list();
    list.open();
    {
        profile_gpu_marker(list, profile_colors::gpu_compute, "%s", name.c_str());

        for (ri_texture* texture : unordered_access_textures)
        {
            list.barrier(*texture, ri_resource_state::initial, ri_resource_state::unordered_access);
        }

        // Resolve all the param blocks we are going to use.
        std::vector<ri_param_block*> blocks = param_blocks; 
        if (view)
        {
            blocks = bind_param_blocks(view->get_resource_cache());
        }

        // Put together param block list to use.
        list.set_pipeline(*technique->pipeline);
        list.set_param_blocks(blocks);
        list.dispatch_rays(dispatch_size.x, dispatch_size.y, dispatch_size.z);

        for (ri_texture* texture : unordered_access_textures)
        {
            list.barrier(*texture, ri_resource_state::unordered_access, ri_resource_state::initial);
        }
    }
    list.close();

    state_output.graphics_command_lists.push_back(&list);
}

}; // namespace ws
