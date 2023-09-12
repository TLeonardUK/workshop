// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"
#include "workshop.core/math/vector2.h"

namespace ws {

// ================================================================================================
//  Types of physics interface implementations available. Make sure to update if you add new ones.
// ================================================================================================
enum class physics_interface_type
{
    jolt
};

// ================================================================================================
//  Engine interface for input implementation.
// ================================================================================================
class physics_interface
{
public:

    // Registers all the steps required to initialize the system.
    // Interacting with this class without successfully running these steps is undefined.
    virtual void register_init(init_list& list) = 0;

};

}; // namespace ws
