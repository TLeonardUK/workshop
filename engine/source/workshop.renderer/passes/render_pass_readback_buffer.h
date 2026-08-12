// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/passes/render_pass_compute.h"
#include "workshop.renderer/render_effect.h"
#include "workshop.renderer/render_output.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface/ri_param_block.h"

namespace ws {

class renderer;

// ================================================================================================
//  Render pass that reads back a render buffer to a cpu-mappable buffer.
// ================================================================================================
class render_pass_readback_buffer
    : public render_pass_compute
{
public:
    ri_buffer* source_buffer;
    ri_buffer* destination_buffer;
    bool clear_source = false;
    
public:
    virtual void generate(renderer& renderer, generated_state& output, render_view* view) override;

private:

};

}; // namespace ws
