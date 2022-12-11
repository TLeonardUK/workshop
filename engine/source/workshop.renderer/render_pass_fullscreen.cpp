// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_pass_fullscreen.h"
#include "workshop.renderer/render_world_state.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/common_types.h"
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

    m_vertex_buffer = factory->create_vertex_buffer("Fullscreen Pass - Vertex buffer");
    m_index_buffer  = factory->create_index_buffer("Fullscreen Pass - Index buffer", std::vector<uint16_t> { 2, 1, 0, 3, 1, 2 });

    // Validate the parameters we've been passed.
    if (result<void> ret = validate_parameters(); !ret)
    {
        return ret;
    }

    return true;
}

result<void> render_pass_fullscreen::destroy_resources(renderer& renderer)
{
    m_index_buffer = nullptr;
    m_vertex_buffer = nullptr;

    return true;
}

result<void> render_pass_fullscreen::validate_parameters()
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
    // TODO
}

void render_pass_fullscreen::generate(renderer& renderer, generated_state& state_output, render_view& view)
{
    ri_command_list& list = renderer.get_render_interface().get_graphics_queue().alloc_command_list();
    list.open();
    list.set_pipeline(*technique->pipeline.get());
    list.set_render_targets(output.color_targets, output.depth_target);
    list.set_viewport(view.viewport);
    list.set_scissor(view.viewport);

    // Needs to bind gbuffer / vertex data.
    //list.set_param_blocks();

    list.set_primitive_topology(ri_primitive::triangle_list);
    list.set_index_buffer(*m_index_buffer);
    list.draw(6, 1);
    list.close();

    state_output.graphics_command_lists.push_back(&list);
}

}; // namespace ws
