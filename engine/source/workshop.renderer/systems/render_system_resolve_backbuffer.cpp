// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_resolve_backbuffer.h"
#include "workshop.renderer/systems/render_system_lighting.h"
#include "workshop.renderer/systems/render_system_raytrace_scene.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_cvars.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"
#include "workshop.renderer/passes/render_pass_compute.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.render_interface/ri_interface.h"

namespace ws {

render_system_resolve_backbuffer::render_system_resolve_backbuffer(renderer& render)
    : render_system(render, "resolve backbuffer")
{
}

void render_system_resolve_backbuffer::register_init(init_list& list)
{
    list.add_step(
        "Resolve Resources",
        [this, &list]() -> result<void> { return create_resources(); },
        [this, &list]() -> result<void> { return destroy_resources(); }
    );
}

result<void> render_system_resolve_backbuffer::create_resources()
{
    size_t histogram_size = 256;

    render_effect::technique* technique = m_renderer.get_effect_manager().get_technique("calculate_luminance_histogram", {});
    technique->get_define("HISTOGRAM_SIZE", histogram_size);

    std::vector<uint8_t> data;
    data.resize(4 * histogram_size);

    ri_buffer::create_params buffer_params;
    buffer_params.element_count = histogram_size;
    buffer_params.element_size = 4;
    buffer_params.usage = ri_buffer_usage::generic;
    m_luminance_histogram_buffer = m_renderer.get_render_interface().create_buffer(buffer_params, "luminance histogram");

    data.resize(4);

    buffer_params.element_count = 1;
    buffer_params.element_size = 4;
    buffer_params.usage = ri_buffer_usage::generic;
    buffer_params.linear_data = data;
    m_luminance_average_buffer = m_renderer.get_render_interface().create_buffer(buffer_params, "luminance average");

    return true;
}

result<void> render_system_resolve_backbuffer::destroy_resources()
{
    return true;
}

void render_system_resolve_backbuffer::build_graph(render_graph& graph, const render_world_state& state, render_view& view)
{
    if (!view.has_flag(render_view_flags::normal))
    {
        return;
    }

    // Calculate luminance calculation parameters.
    const float min_luminance = cvar_eye_adapation_min_luminance.get(); 
    const float max_luminance = cvar_eye_adapation_max_luminance.get();
    const float exposure_tau = cvar_eye_adapation_exposure_tau.get();
    const float white_point = cvar_eye_adapation_white_point.get();
    float time_coeff = std::clamp(1.0f - std::exp(-state.time.delta_seconds * exposure_tau), 0.0f, 1.0f);

    ri_param_block* resolve_luminance_params = view.get_resource_cache().find_or_create_param_block(this, "calculate_luminance_params");
    ri_texture_view texture = &m_renderer.get_system<render_system_lighting>()->get_lighting_buffer();
    resolve_luminance_params->set("min_log2_luminance", min_luminance);
    resolve_luminance_params->set("log2_luminance_range", (max_luminance - min_luminance));
    resolve_luminance_params->set("inverse_log2_luminance_range", 1.0f / (max_luminance - min_luminance));
    resolve_luminance_params->set("input_target", texture);
    resolve_luminance_params->set("input_dimensions", vector2i((int)texture.get_width(), (int)texture.get_height()));
    resolve_luminance_params->set("histogram_buffer", *m_luminance_histogram_buffer, true);
    resolve_luminance_params->set("average_buffer", *m_luminance_average_buffer, true);
    resolve_luminance_params->set("time_coeff", time_coeff);

    // Calculate the luminance histogram
    std::unique_ptr<render_pass_compute> histogram_pass = std::make_unique<render_pass_compute>();
    histogram_pass->name = "calculate luminance histogram";
    histogram_pass->system = this;
    histogram_pass->technique = m_renderer.get_effect_manager().get_technique("calculate_luminance_histogram", {});
    histogram_pass->param_blocks.push_back(resolve_luminance_params);
    histogram_pass->dispatch_size_coverage = vector3i(
        (int)texture.get_width(),
        (int)texture.get_height(),
        1
    );
    graph.add_node(std::move(histogram_pass));

    // Calculate the luminance average
    std::unique_ptr<render_pass_compute> average_pass = std::make_unique<render_pass_compute>();
    average_pass->name = "calculate luminance average";
    average_pass->system = this;
    average_pass->technique = m_renderer.get_effect_manager().get_technique("calculate_luminance_average", {});
    average_pass->param_blocks.push_back(resolve_luminance_params);
    graph.add_node(std::move(average_pass));

    // Resolve to final target.
    std::unique_ptr<render_pass_fullscreen> pass = std::make_unique<render_pass_fullscreen>();
    pass->name = "resolve swapchain";
    pass->system = this;

    if (view.has_render_target())
    {   
        pass->output.color_targets.push_back(view.get_render_target());
    }
    else
    {
        pass->output = m_renderer.get_swapchain_output();
    }

    bool is_hdr_output = (pass->output.color_targets[0].texture->get_format() == ri_texture_format::R32G32B32A32_FLOAT);

    if (is_hdr_output)
    {
        pass->technique = m_renderer.get_effect_manager().get_technique("resolve_swapchain", { 
            { "hdr_output", "true" }
        });
    }
    else
    {
        pass->technique = m_renderer.get_effect_manager().get_technique("resolve_swapchain", { 
            { "hdr_output", "false" } 
        });
    }

    ri_param_block* resolve_param_block = view.get_resource_cache().find_or_create_param_block(this, "resolve_parameters");
    if (view.has_flag(render_view_flags::scene_only))
    {
        resolve_param_block->set("visualization_mode", (int)visualization_mode::normal);
    }
    else
    {
        resolve_param_block->set("visualization_mode", (int)m_renderer.get_visualization_mode());
    }
    resolve_param_block->set("light_buffer_texture", m_renderer.get_system<render_system_lighting>()->get_lighting_buffer());
    resolve_param_block->set("light_buffer_sampler", *m_renderer.get_default_sampler(default_sampler_type::color));
    resolve_param_block->set("raytraced_scene_texture", m_renderer.get_system<render_system_raytrace_scene>()->get_output_buffer());
    resolve_param_block->set("raytraced_scene_sampler", *m_renderer.get_default_sampler(default_sampler_type::color));
    resolve_param_block->set("tonemap_enabled", !is_hdr_output);
    resolve_param_block->set("white_point_squared", math::square(white_point));    
    resolve_param_block->set("average_luminance_buffer", *m_luminance_average_buffer, true);
    resolve_param_block->set("uv_scale", vector2(
        (float)view.get_viewport().width / m_renderer.get_gbuffer_output().color_targets[0].texture->get_width(),
        (float)view.get_viewport().height / m_renderer.get_gbuffer_output().color_targets[0].texture->get_height()
    ));

    pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    pass->param_blocks.push_back(resolve_param_block);

    graph.add_node(std::move(pass));
}

void render_system_resolve_backbuffer::step(const render_world_state& state)
{
}

}; // namespace ws
