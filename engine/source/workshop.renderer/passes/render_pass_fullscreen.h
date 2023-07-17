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
//  Render pass that executes a full screen pass with the given effect.
// ================================================================================================
class render_pass_fullscreen
    : public render_pass_graphics
{
public:

    // If set all depth outputs will be cleared to the maximum value.
    bool clear_depth_outputs = false;

    // If set all color targets with be cleared to 0.
    bool clear_color_outputs = false;

public:

    virtual void generate(renderer& renderer, generated_state& output, render_view& view) override;

private:

};

}; // namespace ws
