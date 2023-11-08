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

class ri_query;

// ================================================================================================
//  Render pass that starts or stops a query.
// ================================================================================================
class render_pass_query
    : public render_pass_graphics
{
public:

    // Start of a query.
    bool start = false;

    // Query to manipulate.
    ri_query* query = nullptr;

public:

    virtual void generate(renderer& renderer, generated_state& output, render_view* view) override;

private:

};

}; // namespace ws
