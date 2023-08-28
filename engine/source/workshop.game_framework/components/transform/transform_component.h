// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/component.h"

#include "workshop.core/math/quat.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/math/matrix4.h"
#include "workshop.core/reflection/reflect.h"

namespace ws {

// ================================================================================================
//  Represents the position, scale and rotation of an object in 3d space.
// ================================================================================================
class transform_component : public component
{
public:

    // Hierarchy
    component_ref<transform_component> parent = null_object;
    std::vector<component_ref<transform_component>> children;

    // Transform relative to the parent.
    quat    local_rotation = quat::identity;
    vector3 local_location = vector3::zero;
    vector3 local_scale = vector3::one;

    // Space transformation
    matrix4 local_to_world = matrix4::identity;
    matrix4 world_to_local = matrix4::identity;

    // Transform in world space.
    quat    world_rotation = quat::identity;
    vector3 world_location = vector3::zero;
    vector3 world_scale = vector3::one;

    // If the local transform has been modified and the local-to-world
    // needs to be update.d
    bool is_dirty = true;

    // Generation increases by one each time the transform has been modified.
    size_t generation = 0;

public:

    BEGIN_REFLECT(transform_component, "Transform", component, reflect_class_flags::none)
        REFLECT_FIELD(local_rotation,   "Rotation",       "Rotation releative to parent.")
        REFLECT_FIELD(local_location,   "Location",       "Local releative to parent.")
        REFLECT_FIELD(local_scale,      "Scale",          "Scale releative to parent.")
    END_REFLECT()

};

}; // namespace ws
