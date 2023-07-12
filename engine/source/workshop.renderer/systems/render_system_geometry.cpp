// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_geometry.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/passes/render_pass_geometry.h"

namespace ws {

render_system_geometry::render_system_geometry(renderer& render)
    : render_system(render, "geometry")
{
}

void render_system_geometry::register_init(init_list& list)
{
}

void render_system_geometry::create_graph(render_graph& graph)
{
    // Draw opaque geometry.
    std::unique_ptr<render_pass_geometry> pass = std::make_unique<render_pass_geometry>();
    pass->name = "opaque static geometry";
    pass->technique = m_renderer.get_effect_manager().get_technique("static_geometry", { {"domain","opaque"}, {"wireframe","false"} });
    pass->wireframe_technique = m_renderer.get_effect_manager().get_technique("static_geometry", { {"wireframe","true"} });
    pass->domain = material_domain::opaque;
    pass->output = m_renderer.get_gbuffer_output();
    pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    graph.add_node(std::move(pass));

    // Draw masked geometry.
    pass = std::make_unique<render_pass_geometry>();
    pass->name = "masked static geometry";
    pass->technique = m_renderer.get_effect_manager().get_technique("static_geometry", { {"domain","masked"}, {"wireframe","false"} });
    pass->wireframe_technique = m_renderer.get_effect_manager().get_technique("static_geometry", { {"wireframe","true"} });
    pass->domain = material_domain::masked;
    pass->output = m_renderer.get_gbuffer_output();
    pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    graph.add_node(std::move(pass));

    // Draw transparent geometry.
    pass = std::make_unique<render_pass_geometry>();
    pass->name = "transparent static geometry";
    pass->technique = m_renderer.get_effect_manager().get_technique("static_geometry", { { "domain","transparent"}, {"wireframe","false"} });
    pass->wireframe_technique = m_renderer.get_effect_manager().get_technique("static_geometry", { {"wireframe","true"} });
    pass->domain = material_domain::transparent;
    pass->output = m_renderer.get_gbuffer_output();
    pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    graph.add_node(std::move(pass));
}

void render_system_geometry::step(const render_world_state& state)
{
}

}; // namespace ws
