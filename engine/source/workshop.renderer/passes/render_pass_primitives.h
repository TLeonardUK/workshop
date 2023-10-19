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
//  Render pass that just draws a given set of primitives.
// ================================================================================================
class render_pass_primitives
    : public render_pass_graphics
{
public:

    ri_buffer* position_buffer;
    ri_buffer* color0_buffer;
    ri_buffer* index_buffer;
    size_t vertex_count = 0;

public:

    virtual void generate(renderer& renderer, generated_state& output, render_view* view) override;

private:

};

}; // namespace ws
