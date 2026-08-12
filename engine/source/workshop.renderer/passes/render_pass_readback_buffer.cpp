// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/passes/render_pass_readback_buffer.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/common_types.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_command_queue.h"
#include "workshop.render_interface/ri_command_list.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface/ri_layout_factory.h"

namespace ws {

void render_pass_readback_buffer::generate(renderer& renderer, generated_state& state_output, render_view* view)
{
    ri_command_list& list = renderer.get_render_interface().get_graphics_queue().alloc_command_list();
    list.open();
    {
        list.barrier(*source_buffer, source_buffer->get_initial_state(), ri_resource_state::copy_source);
        list.barrier(*destination_buffer, destination_buffer->get_initial_state(), ri_resource_state::copy_dest);

        list.copy_buffer(destination_buffer, source_buffer);

        if (clear_source)
        {
            list.barrier(*source_buffer, ri_resource_state::copy_source, ri_resource_state::unordered_access);
            list.barrier(*destination_buffer, ri_resource_state::copy_dest, destination_buffer->get_initial_state());

            list.clear_buffer(source_buffer);

            list.barrier(*source_buffer, ri_resource_state::unordered_access, source_buffer->get_initial_state());
        }
        else
        {
            list.barrier(*source_buffer, ri_resource_state::copy_source, source_buffer->get_initial_state());
            list.barrier(*destination_buffer, ri_resource_state::copy_dest, destination_buffer->get_initial_state());
        }
    }
    list.close();

    state_output.graphics_command_lists.push_back(&list);
}

}; // namespace ws
