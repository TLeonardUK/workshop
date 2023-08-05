// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/renderer.h"
#include "workshop.renderer/passes/render_pass_graphics.h"
#include "workshop.renderer/render_effect.h"
#include "workshop.renderer/render_output.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface/ri_param_block.h"

namespace ws {

// ================================================================================================
//  Render pass that executes a compute shader.
// ================================================================================================
class render_pass_compute
    : public render_pass_graphics
{
public:

    // Overrides the dispatch size with a value calculated as:
    //  ceil(dispatch_size_coverage / group_size)
    // 
    // This is useful if you want to have a group size of say 16x16x1 and want
    // to dispatch enough blocks that it would have enough threads to cover say a 1080x1920 image.
    // Handy for post-processing style functionality.
    vector3i dispatch_size_coverage = vector3i::zero;
    
public:

    virtual void generate(renderer& renderer, generated_state& output, render_view* view) override;

private:

};

}; // namespace ws
