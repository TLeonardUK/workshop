// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/cvar/cvar.h"
#include "workshop.core/platform/platform.h"
#include "workshop.physics_interface/physics_interface.h"

namespace ws {

class physics_interface;

void register_physics_cvars(physics_interface& pi_interface);

// ================================================================================================
//  Read-Only properties used for configuration files.
// ================================================================================================

inline cvar<int> cvar_physics_max_bodies(
    cvar_flag::read_only,
    100'000,
    "physics_max_bodies",
    "Maximum number of physics bodies that can be active at once."
);

inline cvar<int> cvar_physics_max_constraints(
    cvar_flag::read_only,
    100'000,
    "physics_max_constraints",
    "Maximum number of physics constraints that can be active at once."
);

inline cvar<int> cvar_physics_collision_steps(
    cvar_flag::read_only,
    2,
    "physics_collision_steps",
    "Number of steps to divide each physics simulation into. Higher numbers may be more stable but will be more expensive."
);

inline cvar<int> cvar_physics_integration_steps(
    cvar_flag::read_only,
    2,
    "physics_integration_steps",
    "Number of steps to divide each physics integration into. Higher numbers may be more stable but will be more expensive."
);

}; // namespace ws
