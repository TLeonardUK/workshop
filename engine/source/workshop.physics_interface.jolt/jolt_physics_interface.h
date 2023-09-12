// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.physics_interface/physics_interface.h"

namespace ws {

// ================================================================================================
//  Implementation of input using the Jolt library.
// ================================================================================================
class jolt_physics_interface : public physics_interface
{
public:
    jolt_physics_interface();

    virtual void register_init(init_list& list) override;

protected:

    result<void> create_jolt(init_list& list);
    result<void> destroy_jolt();

};

}; // namespace ws
