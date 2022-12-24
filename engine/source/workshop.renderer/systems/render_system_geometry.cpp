// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_geometry.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/render_pass_fullscreen.h"

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
}

void render_system_geometry::step(const render_world_state& state)
{
}

}; // namespace ws
