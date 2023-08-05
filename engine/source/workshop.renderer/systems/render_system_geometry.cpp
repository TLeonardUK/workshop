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

void render_system_geometry::build_graph(render_graph& graph, const render_world_state& state, render_view& view)
{
    // Standard pass.
    if (view.has_flag(render_view_flags::normal))
    {
        // Draw opaque geometry.
        std::unique_ptr<render_pass_geometry> pass = std::make_unique<render_pass_geometry>();
        pass->name = "opaque static geometry";
        pass->system = this;
        pass->technique = m_renderer.get_effect_manager().get_technique("static_geometry", { 
            {"domain","opaque"}, 
            {"wireframe","false"}, 
            {"depth_only","false"} 
        });
        pass->wireframe_technique = m_renderer.get_effect_manager().get_technique("static_geometry", { 
            {"wireframe","true"}, 
            {"depth_only","false"} 
        });
        pass->domain = material_domain::opaque;
        pass->output = m_renderer.get_gbuffer_output();
        pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
        graph.add_node(std::move(pass));

        // Draw masked geometry.
        pass = std::make_unique<render_pass_geometry>();
        pass->name = "masked static geometry";
        pass->system = this;
        pass->technique = m_renderer.get_effect_manager().get_technique("static_geometry", { 
            {"domain","masked"}, 
            {"wireframe","false"}, 
            {"depth_only","false"} 
        });
        pass->wireframe_technique = m_renderer.get_effect_manager().get_technique("static_geometry", { 
            {"wireframe","true"}, 
            {"depth_only","false"} 
        });
        pass->domain = material_domain::masked;
        pass->output = m_renderer.get_gbuffer_output();
        pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
        graph.add_node(std::move(pass));

#if 0
        // Draw transparent geometry.
        pass = std::make_unique<render_pass_geometry>();
        pass->name = "transparent static geometry";
        pass->system = this;
        pass->technique = m_renderer.get_effect_manager().get_technique("static_geometry", { 
            { "domain","transparent"}, 
            {"wireframe","false"}, 
            {"depth_only","false"} 
        });
        pass->wireframe_technique = m_renderer.get_effect_manager().get_technique("static_geometry", { 
            {"wireframe","true"}, 
            {"depth_only","false"} 
        });
        pass->domain = material_domain::transparent;
        pass->output = m_renderer.get_gbuffer_output();
        pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
        graph.add_node(std::move(pass));
#endif

        // Draw sky geometry.
        pass = std::make_unique<render_pass_geometry>();
        pass->name = "sky geometry";
        pass->system = this;
        pass->technique = m_renderer.get_effect_manager().get_technique("sky_box", {});
        pass->info_param_block_type = "geometry_skybox_info";
        pass->domain = material_domain::sky;
        pass->output = m_renderer.get_gbuffer_output();
        pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
        graph.add_node(std::move(pass));
    }
    // Depth-only pass
    else if (view.has_flag(render_view_flags::depth_only) || 
             view.has_flag(render_view_flags::linear_depth_only))
    {
        render_output output;
        if (view.has_render_target())
        {
            output.depth_target = view.get_render_target();
        }
        else
        {
            output.depth_target = m_renderer.get_gbuffer_output().depth_target;
        }

        std::string linear_depth_parameter = "false";
        if (view.has_flag(render_view_flags::linear_depth_only))
        {
            linear_depth_parameter = "true";
        }

        // Draw opaque geometry.
        std::unique_ptr<render_pass_geometry> pass = std::make_unique<render_pass_geometry>();
        pass->name = "opaque static geometry (depth only)";
        pass->system = this;
        pass->technique = m_renderer.get_effect_manager().get_technique("static_geometry", { 
            {"domain","opaque"}, 
            {"wireframe","false"}, 
            {"depth_only","true"}, 
            {"depth_linear",linear_depth_parameter}
        });
        pass->wireframe_technique = m_renderer.get_effect_manager().get_technique("static_geometry", { 
            {"wireframe","true"}, 
            {"depth_only","true"},
            {"depth_linear",linear_depth_parameter}
        });
        pass->domain = material_domain::opaque;
        pass->output = output;
        pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
        graph.add_node(std::move(pass));

        // Draw masked geometry.
        pass = std::make_unique<render_pass_geometry>();
        pass->name = "masked static geometry (depth only)";
        pass->system = this;
        pass->technique = m_renderer.get_effect_manager().get_technique("static_geometry", { 
            {"domain","masked"}, 
            {"wireframe","false"}, 
            {"depth_only","true"},
            {"depth_linear",linear_depth_parameter}
        });
        pass->wireframe_technique = m_renderer.get_effect_manager().get_technique("static_geometry", { 
            {"wireframe","true"}, 
            {"depth_only","true"},
            {"depth_linear",linear_depth_parameter}
        });
        pass->domain = material_domain::masked;
        pass->output = output;
        pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
        graph.add_node(std::move(pass));

        // TODO: Do we want to handle transparent objects here?
    }
}

void render_system_geometry::step(const render_world_state& state)
{
}

}; // namespace ws
