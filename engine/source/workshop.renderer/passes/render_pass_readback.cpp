// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/passes/render_pass_readback.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/common_types.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_command_queue.h"
#include "workshop.render_interface/ri_command_list.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface/ri_layout_factory.h"

namespace ws {

void render_pass_readback::generate(renderer& renderer, generated_state& state_output, render_view* view)
{
    db_assert(readback_pixmap->get_format() == pixmap_format::R8G8B8A8_SRGB);

    ri_command_list& list = renderer.get_render_interface().get_graphics_queue().alloc_command_list();
    list.open();
    {
        list.barrier(*render_target, render_target->get_initial_state(), ri_resource_state::copy_source);
        list.barrier(*readback_buffer, readback_buffer->get_initial_state(), ri_resource_state::copy_dest);

        list.copy_texture(render_target, readback_buffer);

        list.barrier(*render_target, ri_resource_state::copy_source, render_target->get_initial_state());
        list.barrier(*readback_buffer, ri_resource_state::copy_dest, readback_buffer->get_initial_state());
    }
    list.close();

    state_output.graphics_command_lists.push_back(&list);

    renderer.queue_frame_complete_callback([rt=render_target, src=readback_buffer, dst=readback_pixmap]() {

        size_t expected_size = dst->get_width() * dst->get_height() * 4;

        uint8_t* texture_ptr = (uint8_t*)src->map(0, expected_size);
        uint8_t* row_ptr = texture_ptr;

        for (size_t y = 0; y < dst->get_height(); y++)
        {
            for (size_t x = 0; x < dst->get_width(); x++)
            {
                uint8_t* pixel_ptr = row_ptr + (x * 4);

                color pixel_color(
                    (int)pixel_ptr[0], 
                    (int)pixel_ptr[1], 
                    (int)pixel_ptr[2], 
                    (int)pixel_ptr[3]
                );
                dst->set(x, y, pixel_color);
            }

            row_ptr += rt->get_pitch();
        }

        src->unmap(texture_ptr);

    });
}

}; // namespace ws
