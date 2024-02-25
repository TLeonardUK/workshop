// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/hashing/string_hash.h"
#include "workshop.physics_interface/pi_body.h"
#include "workshop.physics_interface/pi_types.h"

namespace ws {

class frame_time;

// ================================================================================================
//  Represents an independently simulatable envioronment in the physics system.
// ================================================================================================
class pi_world
{
public:
    struct create_params
    {
        std::vector<pi_collision_type> collision_types;
    };

public:
    virtual ~pi_world() {}

    // Gets the name this world was created with.
    virtual const char* get_debug_name() = 0;
    
    // Steps physics world one simulation step.
    virtual void step(const frame_time& time) = 0;


    // Creates a new physics body. 
    // The body is not active in the world until add_body is called. 
    virtual std::unique_ptr<pi_body> create_body(const pi_body::create_params& create_params, const char* name) = 0;

    // Adds a body to the world so it participates in simulation.
    virtual void add_body(pi_body& body) = 0;

    // Removes a body from the world so it no longer participates in simulation.
    virtual void remove_body(pi_body& body) = 0;

/*
    // Casts a ray in the physics scene and returns information about the first object hit.
    virtual cast_hit_result ray_cast(
        const vector3& start, 
        const vector3& end, 
        string_hash collision_type_id,
        std::vector<pi_body*> exclude_bodies = {},
        std::vector<pi_body*> include_bodies = {}) = 0;

    // Sweeps a shape through the physics scene and returns information about the first object hit.
    virtual cast_hit_result shape_cast(
        const vector3& start, 
        const vector3& end, 
        const pi_shape& shape,
        string_hash collision_type_id,
        std::vector<pi_body*> exclude_bodies = {},
        std::vector<pi_body*> include_bodies = {}) = 0;
*/
};

}; // namespace ws
