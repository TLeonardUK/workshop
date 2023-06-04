// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_clear.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"
#include "workshop.renderer/render_effect_manager.h"

namespace ws {

render_system_clear::render_system_clear(renderer& render)
    : render_system(render, "clear gbuffer")
{
}

void render_system_clear::register_init(init_list& list)
{
}

void render_system_clear::create_graph(render_graph& graph)
{
    std::unique_ptr<render_pass_fullscreen> pass = std::make_unique<render_pass_fullscreen>();
    pass->name = "clear gbuffer";
    pass->technique = m_renderer.get_effect_manager().get_technique("clear", {});
    pass->output = m_renderer.get_gbuffer_output();
    pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    pass->clear_depth_outputs = true;
  
    graph.add_node(std::move(pass));
}

void render_system_clear::step(const render_world_state& state)
{
}

}; // namespace ws
