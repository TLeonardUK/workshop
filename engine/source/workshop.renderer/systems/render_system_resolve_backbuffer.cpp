// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_resolve_backbuffer.h"
#include "workshop.renderer/systems/render_system_lighting.h"
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

void render_system_resolve_backbuffer::build_graph(render_graph& graph, const render_world_state& state, render_view& view)
{
    if (!view.has_flag(render_view_flags::normal))
    {
        return;
    }

    ri_param_block* resolve_param_block = view.get_resource_cache().find_or_create_param_block(this, "resolve_parameters");
    resolve_param_block->set("visualization_mode", (int)m_renderer.get_visualization_mode());
    resolve_param_block->set("light_buffer_texture", m_renderer.get_system<render_system_lighting>()->get_lighting_buffer());
    resolve_param_block->set("light_buffer_sampler", *m_renderer.get_default_sampler(default_sampler_type::color));

    std::unique_ptr<render_pass_fullscreen> pass = std::make_unique<render_pass_fullscreen>();
    pass->name = "resolve swapchain";
    pass->system = this;
    pass->technique = m_renderer.get_effect_manager().get_technique("resolve_swapchain", {});

    if (ri_texture* target = view.get_render_target())
    {   
        pass->output.color_targets.push_back(target);
    }
    else
    {
        pass->output = m_renderer.get_swapchain_output();
    }

    pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    pass->param_blocks.push_back(resolve_param_block);

    graph.add_node(std::move(pass));
}

void render_system_resolve_backbuffer::step(const render_world_state& state)
{
}

}; // namespace ws
