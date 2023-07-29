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
//  Render pass that simply invokes a callback for something else to handle the 
//  command list generation.
// 
//  This is typically used for graphics systems that are either trivial or specialized enough
//  that making a pass just adds overhead for no direct benefit.
// ================================================================================================
class render_pass_callback
    : public render_pass
{
public:

    using callback_type_t = std::function<void(renderer& renderer, generated_state& output, render_view* view)>;

    callback_type_t callback;

public:

    virtual void generate(renderer& renderer, generated_state& output, render_view* view) override;

};

}; // namespace ws
