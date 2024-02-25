// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/component.h"

#include "workshop.core/math/quat.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/reflection/reflect.h"

#include "workshop.physics_interface/pi_body.h"

namespace ws {

// ================================================================================================
//  Base component that all physics driven objects need to have. 
//  Each object also requires one of the physics shape components (physics_box_component / etc)
// ================================================================================================
class physics_component : public component
{
public:
    
    // The type of collision this physics object uses.
    // TODO: Replace with some kind of hard-typed value for the editor.
    std::string collision_type = "";

    // If true this physics component is expected to move, if false its static and additional
    // optimizations are applied to it.
    bool dynamic = false;

private:

	friend class physics_system;

    std::unique_ptr<pi_body> physics_body;

    // Last known transform change.
    vector3 last_world_location = vector3::zero;
    quat last_world_rotation = quat::identity;
    
    // Last known object scale, will be used to recreate physics body if scale changes.
    vector3 last_world_scale = vector3::one;

    bool is_dirty = false;

public:

    BEGIN_REFLECT(physics_component, "Physics", component, reflect_class_flags::internal_added)
        REFLECT_FIELD(collision_type,   "Collision Type",   "Type of collision to use for this component.");
        REFLECT_FIELD(dynamic,          "Dynamic",          "If this component is expected to move or not, additional optimizations are applied to static components.");
    END_REFLECT()

};

}; // namespace ws
