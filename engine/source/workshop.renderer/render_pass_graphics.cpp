// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_pass_graphics.h"
#include "workshop.renderer/render_world_state.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/common_types.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_command_queue.h"
#include "workshop.render_interface/ri_command_list.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface/ri_layout_factory.h"

namespace ws {

result<void> render_pass_graphics::validate_parameters()
{
    ri_pipeline::create_params pipeline_params = technique->pipeline->get_create_params();

    // Check color targets match.
    if (output.color_targets.size() != pipeline_params.color_formats.size())
    {
        db_error(renderer, "Incorrect number of color output targets in render pass '%s' for technique '%s', got %zi expected %zi.", 
            name.c_str(), 
            technique->name.c_str(),
            output.color_targets.size(),
            pipeline_params.color_formats.size()
        );
        return false;
    }

    for (size_t i = 0; i < output.color_targets.size(); i++)
    {
        ri_texture_format format = output.color_targets[i]->get_format();
        ri_texture_format expected_format = pipeline_params.color_formats[i];

        if (format != expected_format)
        {
            db_error(renderer, "Render pass '%s' using technique '%s' expected color target %zi to be format '%s' but got '%s'.",
                name.c_str(),
                technique->name.c_str(),
                i,
                to_string<ri_texture_format>(expected_format).c_str(),
                to_string<ri_texture_format>(format).c_str()
            );
            return false;
        }
    }

    // Check depth target matches.
    if (pipeline_params.depth_format != ri_texture_format::Undefined)
    {
        if (output.depth_target == nullptr)
        {
            db_error(renderer, "Render pass '%s' has no depth target assigned, but one is expected by technique '%s'.",
                name.c_str(),
                technique->name.c_str()
            );
            return false;
        }

        if (output.depth_target->get_format() != pipeline_params.depth_format)
        {
            db_error(renderer, "Render pass '%s' using technique '%s' expected depth format '%s' but got '%s'.",
                name.c_str(),
                technique->name.c_str(),
                to_string<ri_texture_format>(pipeline_params.depth_format).c_str(),
                to_string<ri_texture_format>(output.depth_target->get_format()).c_str()
            );
            return false;
        }
    }
    else
    {
        if (output.depth_target != nullptr)
        {
            db_error(renderer, "Render pass '%s' has depth target assigned, but no depth target is expected by technique '%s'.",
                name.c_str(),
                technique->name.c_str()
            );
            return false;
        }
    }

    // Check param block types match.
    if (pipeline_params.param_block_archetypes.size() != param_blocks.size())
    {
        db_error(renderer, "Render pass '%s' has %zi param blocks but pipeline expected %zi.",
            name.c_str(),
            param_blocks.size(),
            pipeline_params.param_block_archetypes.size()
        );
        return false;
    }

    for (ri_param_block_archetype* archetype : pipeline_params.param_block_archetypes)
    {
        bool exists = false;

        for (ri_param_block* block : param_blocks)
        {
            if (block->get_archetype() == archetype)
            {
                exists = true;
                break;
            }
        }

        if (!exists)
        {
            db_error(renderer, "Render pass '%s' does not have expected param block '%s'.",
                name.c_str(),
                archetype->get_name()
            );

            return false;
        }
    }

    return true;
}

}; // namespace ws
