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

    ri_texture::create_params texture_params;
    texture_params.width = m_renderer.get_display_width();
    texture_params.height = m_renderer.get_display_height();
    texture_params.dimensions = ri_texture_dimension::texture_2d;
    texture_params.format = ri_texture_format::R16_FLOAT;
    texture_params.is_render_target = true;
    texture_params.optimal_clear_color = color(1.0f, 0.0f, 0.0f, 0.0f);
    m_ssao_texture = render_interface.create_texture(texture_params, "ssao buffer");

    return true;
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

    ri_param_block* ssao_parameters = view.get_resource_cache().find_or_create_param_block(this, "ssao_parameters");
    ssao_parameters->set("uv_scale", vector2(
        (float)view.get_viewport().width / m_renderer.get_gbuffer_output().color_targets[0].texture->get_width(),
        (float)view.get_viewport().height / m_renderer.get_gbuffer_output().color_targets[0].texture->get_height()
    ));
    
    // Composite the transparent geometry onto the light buffer.
    std::unique_ptr<render_pass_fullscreen> resolve_pass = std::make_unique<render_pass_fullscreen>();
    resolve_pass->name = "ssao";
    resolve_pass->system = this;
    resolve_pass->technique = m_renderer.get_effect_manager().get_technique("calculate_ssao", {});
    resolve_pass->output.color_targets.push_back(m_ssao_texture.get());
    resolve_pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    resolve_pass->param_blocks.push_back(view.get_view_info_param_block());
    resolve_pass->param_blocks.push_back(ssao_parameters);
    graph.add_node(std::move(resolve_pass));
}

void render_system_ssao::step(const render_world_state& state)
{
}

}; // namespace ws
