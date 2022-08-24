// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"

namespace ws {

class render_graph;
class render_world_state;

// ================================================================================================
//  Base class. Derived classes are responsible for handling everything required
//  to render a specific part of the rendering pipeline - shadows/ao/etc.
// ================================================================================================
class render_system
{
public:

    // Registers all the steps required to initialize the system.
    virtual void register_init(init_list& list) = 0;

    // Called during setup of the rendering pipeline. During this call the system
    // should insert whatever render passes it needs into the render graph.
    virtual void create_graph(render_graph& graph) = 0;

    // Called each frame. Responsible for doing things like updating uniforms
    // using during rendering.
    virtual void step(const render_world_state& state) = 0;


};

}; // namespace ws
