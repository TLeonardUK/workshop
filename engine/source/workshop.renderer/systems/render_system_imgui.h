// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/render_system.h"

namespace ws {

class renderer;

// ================================================================================================
//  Renders the imgui context to the composited RT.
// ================================================================================================
class render_system_imgui
    : public render_system
{
public:
    render_system_imgui(renderer& render);

    virtual void register_init(init_list& list) override;
    virtual void create_graph(render_graph& graph) override;
    virtual void step(const render_world_state& state) override;

private:

};

}; // namespace ws
