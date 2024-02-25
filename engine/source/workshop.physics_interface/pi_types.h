// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/hashing/string_hash.h"
#include "workshop.core/math/aabb.h"

namespace ws {

class frame_time;

// Represents a collision type an object can have, which dictates how it iteracts with other objects.
struct pi_collision_type
{
    // Identifier for the collision type.
    string_hash id;

    // Identifiers of all collision types that this type will collide solidly with.
    std::vector<string_hash> collides_with;

    // Identifiers of all collision types that this type will register overlaps for, but won't behave 
    // as though they are solid.
    std::vector<string_hash> overlaps_with;
};

// Defines the shape of a physics body.
struct pi_shape
{
    enum class type
    {
        capsule,
        sphere,
        box
    };

    type shape;
    float radius;
    float height;
    vector3 extents;
};

}; // namespace ws
