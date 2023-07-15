// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"

namespace ws {

class renderer;
class render_graph;
class render_world_state;
class render_view;

// ================================================================================================
//  Base class. Derived classes are responsible for handling everything required
//  to render a specific part of the rendering pipeline - shadows/ao/etc.
// ================================================================================================
class render_system
{
public:
    
    // Descriptive name of the system.
    std::string name;

public:

    render_system(renderer& render, const char* name);

    // Registers all the steps required to initialize the system.
    virtual void register_init(init_list& list) = 0;

    // Called during setup of the rendering pipeline. During this call the system
    // should insert whatever render passes it needs into the render graph.
    virtual void create_graph(render_graph& graph) = 0;

    // Called each frame. Responsible for doing things like updating uniforms
    // using during rendering.
    // //
    // This is run in parallel with all other passes, so care must be taken
    // with what it accesses.
    virtual void step(const render_world_state& state) = 0;

    // Called once each frame just prior to a given view being rendered.
    // 
    // This is run in parallel with the rendering of all other views, so care
    // must be taken with what it accesses. 
    virtual void step_view(const render_world_state& state, render_view& view) {};

protected:

    renderer& m_renderer;

};

}; // namespace ws
