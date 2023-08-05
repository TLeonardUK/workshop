// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_transparent_geometry.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/passes/render_pass_geometry.h"
#include "workshop.renderer/systems/render_system_lighting.h"

namespace ws {

render_system_transparent_geometry::render_system_transparent_geometry(renderer& render)
    : render_system(render, "transparent geometry")
{
}

void render_system_transparent_geometry::register_init(init_list& list)
{
}

void render_system_transparent_geometry::build_graph(render_graph& graph, const render_world_state& state, render_view& view)
{
    if (!view.has_flag(render_view_flags::normal))
    {
        return;
    }

    render_system_lighting* lighting_system = m_renderer.get_system<render_system_lighting>();

    render_output output;
    output.color_targets.push_back(&lighting_system->get_lighting_buffer());
    output.depth_target = m_renderer.get_gbuffer_output().depth_target;

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
}

void render_system_transparent_geometry::step(const render_world_state& state)
{
}

}; // namespace ws
