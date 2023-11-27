// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/passes/render_pass_calculate_mips.h"
#include "workshop.renderer/render_world_state.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/common_types.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_command_queue.h"
#include "workshop.render_interface/ri_command_list.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface/ri_layout_factory.h"

namespace ws {

void render_pass_calculate_mips::generate(renderer& renderer, generated_state& state_output, render_view* view)
{
    size_t group_size_x;
    size_t group_size_y;
    size_t group_size_z;

    // Select the correct technique.
    technique = renderer.get_effect_manager().get_technique("calculate_mips", { });

    if (!technique->get_define<size_t>("GROUP_SIZE_X", group_size_x) ||
        !technique->get_define<size_t>("GROUP_SIZE_Y", group_size_y) ||
        !technique->get_define<size_t>("GROUP_SIZE_Z", group_size_z))
    {
        db_warning(renderer, "Failed to run '%s', shader is missing group size defines - GROUP_SIZE_X, GROUP_SIZE_Y, GROUP_SIZE_Z.", technique->name.c_str());
        return;
    }
    
    // Create the command list.
    ri_command_list& list = renderer.get_render_interface().get_graphics_queue().alloc_command_list();
    list.open();
    {
        list.set_pipeline(*technique->pipeline);

        list.barrier(*texture, texture->get_initial_state(), ri_resource_state::unordered_access);

        for (size_t face = 0; face < texture->get_depth(); face++)
        {
            for (size_t mip = 1; mip < texture->get_mip_levels(); mip++)
            {
                ri_texture_view source_view;
                source_view.texture = texture;
                source_view.slice = face;
                source_view.mip = mip - 1;

                ri_texture_view dest_view;
                dest_view.texture = texture;
                dest_view.slice = face;
                dest_view.mip = mip;

                size_t dest_width = dest_view.get_width();
                size_t dest_height = dest_view.get_height();

                std::unique_ptr<ri_param_block> block = renderer.get_param_block_manager().create_param_block("calculate_mips_params");
                block->set("source_texture", source_view, false);
                block->set("source_sampler", *renderer.get_default_sampler(default_sampler_type::color));
                block->set("dest_texture", dest_view, true);
                block->set("texel_size", vector2(1.0f / dest_width, 1.0f / dest_height));

                // Put together param block list to use.
                list.set_param_blocks({ block.get() });
                list.dispatch(
                    std::max(dest_width / group_size_x, 1llu), 
                    std::max(dest_height / group_size_y, 1llu),
                    1
                );
            }
        }

        list.barrier(*texture, ri_resource_state::unordered_access, texture->get_initial_state());
    }
    list.close();

    state_output.graphics_command_lists.push_back(&list);
}

}; // namespace ws
