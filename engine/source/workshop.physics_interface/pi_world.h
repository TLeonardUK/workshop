// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/hashing/string_hash.h"

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

// ================================================================================================
//  Represents an independently simulatable envioronment in the physics system.
// ================================================================================================
class pi_world
{
public:
    virtual ~pi_world() {}

    // Gets the name this world was created with.
    virtual const char* get_debug_name() = 0;

    // Sets the collision types this world uses. 
    // This should be called once at creation, if this is called after the world has been populated 
    // an expensive operation will take place to validate that all bodies have valid collision types.
    virtual void set_collision_types(const std::vector<pi_collision_type>& types) = 0;

    // Steps physics world one simulation step.
    virtual void step(const frame_time& time) = 0;


    /*
        // std::unique_ptr<pi_body> create_body(name)
        // void add_body(body)
        // void remove_body(body)

        // void get_body_transform(body);
        // void set_body_transform();
        // void get_body_shape
        // void set_body_shape
        // void get_body_collision_type
        // void set_body_collision_type

        // hit_result cast_ray(start, end)
        // hit_result cast_shape(start, end, shape)

        // step
    */

};

}; // namespace ws
