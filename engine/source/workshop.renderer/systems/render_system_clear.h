// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/render_system.h"

namespace ws {

class renderer;

// ================================================================================================
//  Resets the gbuffer back to initial state.
// ================================================================================================
class render_system_clear
    : public render_system
{
public:
    render_system_clear(renderer& render);

    virtual void register_init(init_list& list) override;
    virtual void step(const render_world_state& state) override;
    virtual void build_graph(render_graph& graph, const render_world_state& state, render_view& view) override;

private:

};

}; // namespace ws
