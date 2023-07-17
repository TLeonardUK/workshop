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
    : render_system(render, "clear")
{
}

void render_system_clear::register_init(init_list& list)
{
}

void render_system_clear::build_graph(render_graph& graph, const render_world_state& state, render_view& view)
{
    if (view.has_flag(render_view_flags::normal))
    {
        // For normal views do a full gbuffer clear.
        std::unique_ptr<render_pass_fullscreen> pass = std::make_unique<render_pass_fullscreen>();
        pass->name = "clear gbuffer";
        pass->system = this;
        pass->technique = m_renderer.get_effect_manager().get_technique("clear", { });
        pass->output = m_renderer.get_gbuffer_output();
        pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
        pass->clear_depth_outputs = true;  
        graph.add_node(std::move(pass));
    }
    else if (view.has_flag(render_view_flags::depth_only))
    {
        render_output output;
        if (view.get_render_target())
        {
            output.depth_target = view.get_render_target();
        }
        else
        {
            output.depth_target = m_renderer.get_gbuffer_output().depth_target;
        }

        // For normal views do a full gbuffer clear.
        std::unique_ptr<render_pass_fullscreen> pass = std::make_unique<render_pass_fullscreen>();
        pass->name = "clear depth";
        pass->system = this;
        pass->technique = nullptr;
        pass->output = output;
        pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
        pass->clear_depth_outputs = true;
        graph.add_node(std::move(pass));
    }
}

void render_system_clear::step(const render_world_state& state)
{
}

}; // namespace ws
