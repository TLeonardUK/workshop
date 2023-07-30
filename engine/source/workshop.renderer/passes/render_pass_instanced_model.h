// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/renderer.h"
#include "workshop.renderer/passes/render_pass_graphics.h"
#include "workshop.renderer/render_effect.h"
#include "workshop.renderer/render_output.h"
#include "workshop.renderer/assets/model/model.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface/ri_param_block.h"

namespace ws {

class material;
class statistics_channel;

// ================================================================================================
//  Render pass that draws multiple instances of a single model. This is mostly here for debug
//  functionality, most drawing should go via render_pass_geometry
// ================================================================================================
class render_pass_instanced_model
    : public render_pass_graphics
{
public:

    // Model to be rendered.
    asset_ptr<model> render_model;

    // Param block for each instance to be rendered.
    std::vector<ri_param_block*> instances;


public:

    render_pass_instanced_model();

    virtual void generate(renderer& renderer, generated_state& output, render_view* view) override;

private:
    statistics_channel* m_stats_triangles_rendered;
    statistics_channel* m_stats_draw_calls;
    statistics_channel* m_stats_drawn_instances;
    statistics_channel* m_stats_culled_instances;

};

}; // namespace ws
