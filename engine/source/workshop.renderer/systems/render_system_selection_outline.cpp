// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_selection_outline.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"
#include "workshop.renderer/passes/render_pass_callback.h"
#include "workshop.renderer/passes/render_pass_primitives.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_command_queue.h"
#include "workshop.render_interface/ri_command_list.h"

#include "workshop.core/math/matrix4.h"
#include "workshop.core/perf/profile.h"

#include "workshop.render_interface/ri_interface.h"

namespace ws {

render_system_selection_outline::render_system_selection_outline(renderer& render)
    : render_system(render, "selection outline")
{
}

void render_system_selection_outline::register_init(init_list& list)
{
}

void render_system_selection_outline::build_graph(render_graph& graph, const render_world_state& state, render_view& view)
{
    if (!view.has_flag(render_view_flags::normal) ||
         view.has_flag(render_view_flags::scene_only))
    {
        return;
    }

    ri_param_block* outline_param_block = view.get_resource_cache().find_or_create_param_block(this, "selection_outline_parameters");
    outline_param_block->set("uv_step", vector2(
        1.0f / m_renderer.get_swapchain_output().color_targets[0].get_width(),
        1.0f / m_renderer.get_swapchain_output().color_targets[0].get_height()
    ));
    outline_param_block->set("outline_color", color::gold.rgba());
    outline_param_block->set("fill_alpha", 0.25f);

    // Resolve to final target.
    std::unique_ptr<render_pass_fullscreen> pass = std::make_unique<render_pass_fullscreen>();
    pass->name = "selection outline";
    pass->system = this;
    pass->technique = m_renderer.get_effect_manager().get_technique("render_selection_outline", {});
    if (view.has_render_target())
    {
        pass->output.color_targets.push_back(view.get_render_target());
    }
    else
    {
        pass->output.color_targets = m_renderer.get_swapchain_output().color_targets;
    }
    pass->output.depth_target = m_renderer.get_gbuffer_output().depth_target;
    pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    pass->param_blocks.push_back(outline_param_block);
    graph.add_node(std::move(pass));
}

void render_system_selection_outline::step(const render_world_state& state)
{
}

}; // namespace ws
