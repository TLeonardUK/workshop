// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"

namespace ws {

class window;

// ================================================================================================
//  Types of platform implementations available. Make sure to update if you add new ones.
// ================================================================================================
enum class platform_interface_type
{
    sdl
};

// ================================================================================================
//  Engine interface for all platform implementation. The platform interface incomperates anything
//  related to the process lifecycle but is non-specific to oen of the other main interfaces. 
//  Things like handling OS events, etc.
// ================================================================================================
class platform_interface
{
public:

    // Registers all the steps required to initialize the platform system.
    // Interacting with this class without successfully running these steps is undefined.
    virtual void register_init(init_list& list) = 0;

    // Processes and dispatches any events recieved.
    virtual void pump_events() = 0;

};

}; // namespace ws
