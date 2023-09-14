// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_ssao.h"
#include "workshop.renderer/systems/render_system_lighting.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"
#include "workshop.renderer/passes/render_pass_compute.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.render_interface/ri_interface.h"

#include <random>

namespace ws {

render_system_ssao::render_system_ssao(renderer& render)
    : render_system(render, "ssao")
{
}

void render_system_ssao::register_init(init_list& list)
{
    list.add_step(
        "SSAO Resources",
        [this, &list]() -> result<void> { return create_resources(); },
        [this, &list]() -> result<void> { return destroy_resources(); }
    );
}

result<void> render_system_ssao::create_resources()
{
    ri_interface& render_interface = m_renderer.get_render_interface();

    const render_options& options = m_renderer.get_options();

    ri_texture::create_params texture_params;
    texture_params.width = m_renderer.get_display_width();
    texture_params.height = m_renderer.get_display_height();
    texture_params.dimensions = ri_texture_dimension::texture_2d;
    texture_params.format = ri_texture_format::R16_FLOAT; 
    texture_params.is_render_target = true;
    texture_params.optimal_clear_color = color(1.0f, 0.0f, 0.0f, 0.0f);
    m_ssao_texture = render_interface.create_texture(texture_params, "ssao buffer");
    m_ssao_blur_texture = render_interface.create_texture(texture_params, "ssao blur buffer");

    // Generate a random vector texture.
    std::uniform_real_distribution<float> random_floats(0.0, 1.0);
    std::default_random_engine random_generator;

    std::vector<vector4> noise;
    for (size_t i = 0; i < 16; i++)
    {
        vector4 sample = {
            random_floats(random_generator) * 2.0f - 1.0f,
            random_floats(random_generator) * 2.0f - 1.0f,
            0.0f,
            0.0f
        };
        noise.push_back(sample);
    }

    ri_texture::create_params noise_texture_params;
    noise_texture_params.width = 4;
    noise_texture_params.height = 4;
    noise_texture_params.dimensions = ri_texture_dimension::texture_2d;
    noise_texture_params.format = ri_texture_format::R32G32B32A32_FLOAT;
    noise_texture_params.data = std::span<uint8_t>(reinterpret_cast<uint8_t*>(noise.data()), noise.size() * sizeof(vector4));
    m_noise_texture = render_interface.create_texture(noise_texture_params, "ssao noise buffer");

    ri_sampler::create_params noise_sampler_params;
    noise_sampler_params.filter = ri_texture_filter::nearest_neighbour;
    m_noise_texture_sampler = render_interface.create_sampler(noise_sampler_params, "ssao noise sampler");

    // Useful to generate a kernel to store in the shader.
#if 0
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
    std::default_random_engine generator;
    std::vector<vector3> ssaoKernel;
    for (unsigned int i = 0; i < 32; ++i)
    {
        vector3 sample(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator)
        );
        sample = sample.normalize();
        sample *= randomFloats(generator);

        float scale = (float)i / 16.0f;
        scale = math::lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;

        db_log(renderer, "float3(%.4f, %.4f, %.4f)", sample.x, sample.y, sample.z);
    }
#endif

    return true;
}

void render_system_ssao::swapchain_resized()
{
    bool result = create_resources();
    db_assert(result);
}

result<void> render_system_ssao::destroy_resources()
{
    return true;
}

ri_texture* render_system_ssao::get_ssao_mask()
{
    return m_ssao_texture.get();
}

void render_system_ssao::build_graph(render_graph& graph, const render_world_state& state, render_view& view)
{
    if (!view.has_flag(render_view_flags::normal))
    {
        return;
    }

    const render_options& options = m_renderer.get_options();

    ri_param_block* ssao_parameters = view.get_resource_cache().find_or_create_param_block(this, "ssao_parameters");
    ri_param_block* h_blur_params = view.get_resource_cache().find_or_create_param_block(this, "blur_params");
    ri_param_block* v_blur_params = view.get_resource_cache().find_or_create_param_block(this + 1, "blur_params");

    ssao_parameters->set("uv_scale", vector2(
        (float)view.get_viewport().width / m_renderer.get_gbuffer_output().color_targets[0].texture->get_width(),
        (float)view.get_viewport().height / m_renderer.get_gbuffer_output().color_targets[0].texture->get_height()
    ));
    ssao_parameters->set("noise_texture", *m_noise_texture);
    ssao_parameters->set("noise_texture_sampler", *m_noise_texture_sampler);
    ssao_parameters->set("ssao_radius", options.ssao_sample_radius);
    ssao_parameters->set("ssao_power", options.ssao_intensity_power);
    
    // Composite the transparent geometry onto the light buffer.
    std::unique_ptr<render_pass_fullscreen> resolve_pass = std::make_unique<render_pass_fullscreen>();
    resolve_pass->name = "ssao";
    resolve_pass->system = this;
    resolve_pass->viewport = recti(
        0, 
        0,
        static_cast<size_t>(view.get_viewport().width * options.ssao_resolution_scale),
        static_cast<size_t>(view.get_viewport().height * options.ssao_resolution_scale)
    );
    resolve_pass->technique = m_renderer.get_effect_manager().get_technique("calculate_ssao", {});
    resolve_pass->output.color_targets.push_back(m_ssao_texture.get());
    resolve_pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    resolve_pass->param_blocks.push_back(view.get_view_info_param_block());
    resolve_pass->param_blocks.push_back(ssao_parameters);
    graph.add_node(std::move(resolve_pass));

    // Do horizontal blur.
    h_blur_params->set("input_texture", *m_ssao_texture.get());
    h_blur_params->set("input_texture_sampler", *m_renderer.get_default_sampler(default_sampler_type::color_clamped));
    h_blur_params->set("input_texture_size", vector2i((int)m_ssao_texture->get_width(), (int)m_ssao_texture->get_height()));
    h_blur_params->set("input_uv_scale", vector2(
        (float)view.get_viewport().width / m_ssao_texture->get_width(),
        (float)view.get_viewport().height / m_ssao_texture->get_height()
    ));

    std::unique_ptr<render_pass_fullscreen> h_blur_pass = std::make_unique<render_pass_fullscreen>();
    h_blur_pass->name = "ssao horizontal blur";
    h_blur_pass->system = this;
    h_blur_pass->technique = m_renderer.get_effect_manager().get_technique("blur", {
        { "direction", "x" },
        { "format", "R16_FLOAT" },
        { "radius", "10" }
    });
    h_blur_pass->scissor = recti(
        0,
        0,
        static_cast<size_t>(view.get_viewport().width * options.ssao_resolution_scale),
        static_cast<size_t>(view.get_viewport().height * options.ssao_resolution_scale)
    );
    h_blur_pass->output.color_targets.push_back(m_ssao_blur_texture.get());
    h_blur_pass->param_blocks.push_back(view.get_view_info_param_block());
    h_blur_pass->param_blocks.push_back(h_blur_params);
    graph.add_node(std::move(h_blur_pass));

    // Do vertical blur.
    v_blur_params->set("input_texture", *m_ssao_blur_texture.get());
    v_blur_params->set("input_texture_sampler", *m_renderer.get_default_sampler(default_sampler_type::color_clamped));
    v_blur_params->set("input_texture_size", vector2i((int)m_ssao_blur_texture->get_width(), (int)m_ssao_blur_texture->get_height()));
    v_blur_params->set("input_uv_scale", vector2(
        (float)view.get_viewport().width / m_ssao_texture->get_width(),
        (float)view.get_viewport().height / m_ssao_texture->get_height()
    ));

    std::unique_ptr<render_pass_fullscreen> v_blur_pass = std::make_unique<render_pass_fullscreen>();
    v_blur_pass->name = "ssao vertical blur";
    v_blur_pass->system = this;
    v_blur_pass->technique = m_renderer.get_effect_manager().get_technique("blur", {
        { "direction", "y" },
        { "format", "R16_FLOAT" },
        { "radius", "10" }
    });
    v_blur_pass->scissor = recti(
        0,
        0,
        static_cast<size_t>(view.get_viewport().width * options.ssao_resolution_scale),
        static_cast<size_t>(view.get_viewport().height * options.ssao_resolution_scale)
    );
    v_blur_pass->output.color_targets.push_back(m_ssao_texture.get());
    v_blur_pass->param_blocks.push_back(view.get_view_info_param_block());
    v_blur_pass->param_blocks.push_back(v_blur_params);
    graph.add_node(std::move(v_blur_pass));
}

void render_system_ssao::step(const render_world_state& state)
{
}

}; // namespace ws
