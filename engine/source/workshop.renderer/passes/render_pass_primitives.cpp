// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/passes/render_pass_primitives.h"
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

void render_pass_primitives::generate(renderer& renderer, generated_state& state_output, render_view& view)
{
    ri_param_block* vertex_info_param_block = view.get_resource_cache().find_or_create_param_block(get_cache_key(view), "vertex_info");
    vertex_info_param_block->set("vertex_buffer", *vertex_buffer);
    vertex_info_param_block->set("vertex_buffer_offset", 0u);
    vertex_info_param_block->clear_buffer("instance_buffer");

    ri_command_list& list = renderer.get_render_interface().get_graphics_queue().alloc_command_list();
    list.open();
    {
        profile_gpu_marker(list, profile_colors::gpu_pass, "primitives");

        list.barrier(*output.color_targets[0], ri_resource_state::initial, ri_resource_state::render_target);
        list.barrier(*output.depth_target, ri_resource_state::initial, ri_resource_state::depth_read);
        list.set_pipeline(*technique->pipeline.get());
        list.set_render_targets(output.color_targets, output.depth_target);
        list.set_viewport(view.get_viewport());
        list.set_scissor(view.get_viewport());
        list.set_primitive_topology(ri_primitive::line_list);
        list.set_index_buffer(*index_buffer);
        list.set_param_blocks({
            vertex_info_param_block,
            view.get_view_info_param_block()
            });
        list.draw(vertex_count, 1, 0);

        list.barrier(*output.depth_target, ri_resource_state::depth_read, ri_resource_state::initial);
        list.barrier(*output.color_targets[0], ri_resource_state::render_target, ri_resource_state::initial);

    }
    list.close();

    state_output.graphics_command_lists.push_back(&list);
}

}; // namespace ws
