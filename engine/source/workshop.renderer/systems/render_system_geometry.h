// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/render_system.h"

namespace ws {

class renderer;

// ================================================================================================
//  Renders the scenes geometry to the gbuffer.
// ================================================================================================
class render_system_geometry
    : public render_system
{
public:
    render_system_geometry(renderer& render);

    virtual void register_init(init_list& list) override;
    virtual void create_graph(render_graph& graph) override;
    virtual void step(const render_world_state& state) override;

private:

};

}; // namespace ws
