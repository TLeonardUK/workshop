// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.physics_interface/physics_cvars.h"
#include "workshop.physics_interface/physics_interface.h"

namespace ws {

void register_physics_cvars(physics_interface& pi_interface)
{
    cvar_physics_max_bodies.register_self();
    cvar_physics_max_constraints.register_self();
    cvar_physics_collision_steps.register_self();
    cvar_physics_integration_steps.register_self();
}

}; // namespace ws
