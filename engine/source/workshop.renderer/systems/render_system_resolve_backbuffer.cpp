// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_resolve_backbuffer.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_graph.h"

namespace ws {

render_system_resolve_backbuffer::render_system_resolve_backbuffer(renderer& render)
    : render_system(render, "resolve backbuffer")
{
}

void render_system_resolve_backbuffer::register_init(init_list& list)
{   
    // param_block = create_param_block();
}

void render_system_resolve_backbuffer::create_graph(render_graph& graph)
{
//    std::unique_ptr<render_pass_graphics> graphics_pass = std::make_unique<render_pass_graphics>();
//    graphics_pass->name = "copy to backbuffer";
//    graphics_pass->effect = m_renderer.get_effect("copy_to_backbuffer", { "draw_mode": "Opaque" });
//    graphics_pass->target_clear = true;
//    graphics_pass->target_use_backbuffer = true;
//    graphics_pass->target_set.color[0] = ;
//    graphics_pass->param_blocks.add();

//    graph.add_node(std::move(graphics_pass), { });
}

void render_system_resolve_backbuffer::step(const render_world_state& state)
{
}

}; // namespace ws
