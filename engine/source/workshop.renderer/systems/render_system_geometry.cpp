// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_geometry.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/render_pass_graphics.h"

namespace ws {

render_system_geometry::render_system_geometry(renderer& render)
    : m_renderer(render)
{
}

void render_system_geometry::register_init(init_list& list)
{
}

void render_system_geometry::create_graph(render_graph& graph)
{
    std::unique_ptr<render_pass_graphics> graphics_pass = std::make_unique<render_pass_graphics>();
    graphics_pass->name = "geometry";
    graphics_pass->effect = m_renderer.get_effect("geometry");
    
    graph.add_node(std::move(graphics_pass), { });
}

void render_system_geometry::step(const render_world_state& state)
{
}

}; // namespace ws
