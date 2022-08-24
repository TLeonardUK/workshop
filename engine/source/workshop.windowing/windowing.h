// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"

#include "workshop.render_interface/render_interface.h"
#include "workshop.windowing/window.h"

namespace ws {

class window;

// ================================================================================================
//  Types of windowing implementations available. Make sure to update if you add new ones.
// ================================================================================================
enum class windowing_type
{
    sdl
};

// ================================================================================================
//  Engine interface for all windowing implementation. Windowing encompases creation of windows,
//  pumping of platform messages queues, and anything else required to display and update windows.
// ================================================================================================
class windowing
{
public:

    // Registers all the steps required to initialize the windowing system.
    // Interacting with this class without successfully running these steps is undefined.
    virtual void register_init(init_list& list) = 0;

    // Processes and dispatches any events recieved.
    virtual void pump_events() = 0;

    // Creates a new window with basic settings. 
    // Compatibility value defines what renderers can write to this window. Compatibility is 
    // immutable and requires the window to be recreated to change.
    virtual std::unique_ptr<window> create_window(
        const char* name, 
        size_t width, 
        size_t height, 
        window_mode mode, 
        render_interface_type compatibility) = 0;

};

}; // namespace ws
