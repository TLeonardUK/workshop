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

    // Called each frame. Responsible for doing things like updating uniforms
    // using during rendering.
    // //
    // This is run in parallel with all other passes, so care must be taken
    // with what it accesses.
    virtual void step(const render_world_state& state) = 0;

    // Called once each frame for each view, should create any render passes
    // needed to render the system.
    // 
    // This is run in parallel with the rendering of all other views, so care
    // must be taken with what it accesses. 
    virtual void build_graph(render_graph& graph, const render_world_state& state, render_view& view) {};

    // Called once per frame, generates a graph for rendering that occurs before all view rendering.
    virtual void build_pre_graph(render_graph& graph, const render_world_state& state) {};

    // Called once per frame, generates a graph for rendering that occurs after all view rendering.
    virtual void build_post_graph(render_graph& graph, const render_world_state& state) {};

protected:

    renderer& m_renderer;

};

}; // namespace ws
