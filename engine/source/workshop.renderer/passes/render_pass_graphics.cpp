// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/passes/render_pass_graphics.h"
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
        ri_texture_format format = output.color_targets[i].texture->get_format();
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
        if (output.depth_target.texture == nullptr)
        {
            db_error(renderer, "Render pass '%s' has no depth target assigned, but one is expected by technique '%s'.",
                name.c_str(),
                technique->name.c_str()
            );
            return false;
        }

        if (output.depth_target.texture->get_format() != pipeline_params.depth_format)
        {
            db_error(renderer, "Render pass '%s' using technique '%s' expected depth format '%s' but got '%s'.",
                name.c_str(),
                technique->name.c_str(),
                to_string<ri_texture_format>(pipeline_params.depth_format).c_str(),
                to_string<ri_texture_format>(output.depth_target.texture->get_format()).c_str()
            );
            return false;
        }
    }
    else
    {
        if (output.depth_target.texture != nullptr)
        {
            db_error(renderer, "Render pass '%s' has depth target assigned, but no depth target is expected by technique '%s'.",
                name.c_str(),
                technique->name.c_str()
            );
            return false;
        }
    }

    // Check param block types match.    
    for (ri_param_block_archetype* archetype : pipeline_params.param_block_archetypes)
    {
        // Ignore instanced param buffers, we pass these around in a variety of ways indirectly.
        if (archetype->get_create_params().scope == ri_data_scope::instance)
        {
            continue;
        }

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
            m_runtime_bound_param_blocks.push_back(archetype);
        }
    }

    return true;
}

std::vector<ri_param_block*> render_pass_graphics::bind_param_blocks(render_resource_cache& cache)
{
    std::vector<ri_param_block*> result = param_blocks;

    for (ri_param_block_archetype* archetype : m_runtime_bound_param_blocks)
    {
        ri_param_block* block = cache.find_param_block_by_name(archetype->get_name());
        if (block == nullptr)
        {
            db_fatal(renderer, "Missing param block that was expected to be bound at runtime '%s'.", archetype->get_name());
        }

        result.push_back(block);
    }

    return result;
}

}; // namespace ws
