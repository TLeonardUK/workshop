// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_pass.h"
#include "workshop.renderer/render_effect.h"
#include "workshop.renderer/render_output.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface/ri_param_block.h"

namespace ws {

// ================================================================================================
//  Base class for all graphics render passes.
// ================================================================================================
class render_pass_graphics
    : public render_pass
{
public:

    // The effect technique to use for rendering this pass.
    render_effect::technique* technique;

    // The effect technique to use for rendering this pass in wireframe.
    render_effect::technique* wireframe_technique;

    // The output targets to render to.
    render_output output;

    // The param blocks required by the technique being rendered.
    std::vector<ri_param_block*> param_blocks;

public:
    result<void> validate_parameters();

};

}; // namespace ws
