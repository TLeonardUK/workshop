// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_imgui.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"

namespace ws {

render_system_imgui::render_system_imgui(renderer& render)
    : render_system(render, "imgui")
{
}

void render_system_imgui::register_init(init_list& list)
{
}

void render_system_imgui::create_graph(render_graph& graph)
{
}

void render_system_imgui::step(const render_world_state& state)
{
}

}; // namespace ws
