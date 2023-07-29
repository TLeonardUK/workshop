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
#include "workshop.renderer/systems/render_system_imgui.h"

namespace ws {

// ================================================================================================
//  Render pass that draws a set of imgui draw commands
// ================================================================================================
class render_pass_imgui
    : public render_pass_graphics
{
public:

    ri_texture* default_texture = nullptr;

    ri_buffer* vertex_buffer = nullptr;
    ri_buffer* index_buffer = nullptr;

    std::vector<render_system_imgui::draw_command> draw_commands;

public:

    virtual void generate(renderer& renderer, generated_state& output, render_view* view) override;

private:

};

}; // namespace ws
