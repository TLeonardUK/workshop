// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_transparent_geometry.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/passes/render_pass_geometry.h"
#include "workshop.renderer/passes/render_pass_clear.h"
#include "workshop.renderer/systems/render_system_lighting.h"
#include "workshop.render_interface/ri_interface.h"

namespace ws {

render_system_transparent_geometry::render_system_transparent_geometry(renderer& render)
    : render_system(render, "transparent geometry")
{
}

void render_system_transparent_geometry::register_init(init_list& list)
{
    list.add_step(
        "Transparency Resources",
        [this, &list]() -> result<void> { return create_resources(); },
        [this, &list]() -> result<void> { return destroy_resources(); }
    );
}

result<void> render_system_transparent_geometry::create_resources()
{
    ri_interface& render_interface = m_renderer.get_render_interface();

    ri_texture::create_params texture_params;
    texture_params.width = m_renderer.get_display_width();
    texture_params.height = m_renderer.get_display_height();
    texture_params.dimensions = ri_texture_dimension::texture_2d;
    texture_params.format = ri_texture_format::R16G16B16A16_FLOAT;
    texture_params.is_render_target = true;
    texture_params.optimal_clear_color = color(0.0f, 0.0f, 0.0f, 0.0f);
    m_accumulation_buffer = render_interface.create_texture(texture_params, "transparency accumulation buffer");

    texture_params.format = ri_texture_format::R16_FLOAT;
    texture_params.optimal_clear_color = color(1.0f, 1.0f, 1.0f, 1.0f);
    m_revealance_buffer = render_interface.create_texture(texture_params, "transparency revealence buffer");

    m_resolve_param_block = m_renderer.get_param_block_manager().create_param_block("resolve_transparent_parameters");
    m_resolve_param_block->set("accumulation_texture", *m_accumulation_buffer);
    m_resolve_param_block->set("revealance_texture", *m_revealance_buffer);
    m_resolve_param_block->set("texture_sampler", *m_renderer.get_default_sampler(default_sampler_type::color));

    return true;
}

void render_system_transparent_geometry::swapchain_resized()
{
    bool result = create_resources();
    db_assert(result);
}

result<void> render_system_transparent_geometry::destroy_resources()
{
    return true;
}

void render_system_transparent_geometry::build_graph(render_graph& graph, const render_world_state& state, render_view& view)
{
    if (!view.has_flag(render_view_flags::normal))
    {
        return;
    }
    render_system_lighting* lighting_system = m_renderer.get_system<render_system_lighting>();

    // We use a weighted blended algorith for OIT, reference:
    // https://learnopengl.com/Guest-Articles/2020/OIT/Weighted-Blended

    // Draw transparent geometry to our accumulation buffers.
    render_output output;
    output.color_targets.push_back(m_accumulation_buffer.get());
    output.color_targets.push_back(m_revealance_buffer.get());
    output.depth_target = m_renderer.get_gbuffer_output().depth_target;
    
    // Output for final resolve.
    render_output resolve_output;
    resolve_output.color_targets.push_back(&m_renderer.get_system<render_system_lighting>()->get_lighting_buffer());

    // Clear accumulation/relevance buffers to expected output.
    std::unique_ptr<render_pass_clear> clear_pass = std::make_unique<render_pass_clear>();
    clear_pass->name = "clear transparency buffers";
    clear_pass->output = output;
    clear_pass->output.depth_target = {};
    graph.add_node(std::move(clear_pass));

    // Draw transparent geometry.
    std::unique_ptr<render_pass_geometry> pass = std::make_unique<render_pass_geometry>();
    pass->name = "transparent static geometry";
    pass->system = this;
    pass->technique = m_renderer.get_effect_manager().get_technique("transparent_static_geometry", { 
        { "domain","transparent"}, 
        {"wireframe","false"}, 
        {"depth_only","false"} 
    });
    pass->wireframe_technique = m_renderer.get_effect_manager().get_technique("static_geometry", { 
        {"wireframe","true"}, 
        {"depth_only","false"} 
    });
    pass->domain = material_domain::transparent;
    pass->output = output;
    pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    pass->param_blocks.push_back(view.get_view_info_param_block());
    pass->param_blocks.push_back(&lighting_system->get_resolve_param_block(view));
    graph.add_node(std::move(pass));

    // Composite the transparent geometry onto the light buffer.
    std::unique_ptr<render_pass_fullscreen> resolve_pass = std::make_unique<render_pass_fullscreen>();
    resolve_pass->name = "resolve transparency";
    resolve_pass->system = this;
    resolve_pass->technique = m_renderer.get_effect_manager().get_technique("transparent_resolve", {});
    resolve_pass->output = resolve_output;
    resolve_pass->param_blocks.push_back(m_resolve_param_block.get());
    graph.add_node(std::move(resolve_pass));
}

void render_system_transparent_geometry::step(const render_world_state& state)
{
}

}; // namespace ws
