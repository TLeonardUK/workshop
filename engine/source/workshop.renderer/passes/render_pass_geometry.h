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
//  Render pass that draws a set of geometry visible from the given view. 
// ================================================================================================
class render_pass_geometry
    : public render_pass_graphics
{
public:

    // What material domain to render.
    material_domain domain = material_domain::opaque;

public:

    render_pass_geometry();

    virtual void generate(renderer& renderer, generated_state& output, render_view& view) override;

private:
    statistics_channel* m_stats_triangles_rendered;
    statistics_channel* m_stats_draw_calls;
    statistics_channel* m_stats_drawn_instances;
    statistics_channel* m_stats_culled_instances;

};

}; // namespace ws
