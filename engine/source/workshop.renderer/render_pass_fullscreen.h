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
//  Render pass that executes a full screen pass with the given effect.
// ================================================================================================
class render_pass_fullscreen
    : public render_pass
{
public:

    // The effect technique to use for rendering this pass.
    render_effect::technique* technique;

    // The output targets to render to.
    render_output output;

public:

    virtual result<void> create_resources(renderer& renderer) override;
    virtual result<void> destroy_resources(renderer& renderer) override;

    virtual void generate(renderer& renderer, generated_state& output, render_view& view) override;

    result<void> validate_parameters();

private:
    std::unique_ptr<ri_buffer> m_vertex_buffer;
    std::unique_ptr<ri_buffer> m_index_buffer;

    std::unique_ptr<ri_param_block> m_vertex_info_param_block;

};

}; // namespace ws
