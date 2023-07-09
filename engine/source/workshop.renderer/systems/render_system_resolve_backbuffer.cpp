// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_resolve_backbuffer.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"

namespace ws {

render_system_resolve_backbuffer::render_system_resolve_backbuffer(renderer& render)
    : render_system(render, "resolve backbuffer")
{
}

void render_system_resolve_backbuffer::register_init(init_list& list)
{   
}

void render_system_resolve_backbuffer::create_graph(render_graph& graph)
{
    m_resolve_param_block = m_renderer.get_param_block_manager().create_param_block("resolve_parameters");
    m_resolve_param_block->set("visualization_mode", (int)m_renderer.get_visualization_mode());

    std::unique_ptr<render_pass_fullscreen> pass = std::make_unique<render_pass_fullscreen>();
    pass->name = "resolve swapchain";
    pass->technique = m_renderer.get_effect_manager().get_technique("resolve_swapchain", {});
    pass->output = m_renderer.get_swapchain_output();
    pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    pass->param_blocks.push_back(m_resolve_param_block.get());

    m_render_pass = pass.get();

    graph.add_node(std::move(pass));
}

void render_system_resolve_backbuffer::step(const render_world_state& state)
{
    // Update the target we are outputting to to the current swapchain backbuffer.
    m_render_pass->output = m_renderer.get_swapchain_output();

    // Apply current visualization mode.
    m_resolve_param_block->set("visualization_mode", (int)m_renderer.get_visualization_mode());
}

}; // namespace ws
