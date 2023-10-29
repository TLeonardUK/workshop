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
//  Render pass that executes a raytracing shader
// ================================================================================================
class render_pass_raytracing
    : public render_pass_graphics
{
public:

    // Size of raytracing workgroup to dispatch.
    vector3i dispatch_size = vector3i::zero;

    // Transitions resources to/from unordered access when this pass executes.
    // TODO: Abstract this away, it should already be known from the param blocks.
    std::vector<ri_texture*> unordered_access_textures;

public:

    virtual void generate(renderer& renderer, generated_state& output, render_view* view) override;

private:

};

}; // namespace ws
