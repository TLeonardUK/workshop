// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/hashing/string_hash.h"
#include "workshop.core/math/quat.h"
#include "workshop.physics_interface/pi_types.h"

namespace ws {

class frame_time;

// ================================================================================================
//  Represents a rigid body that participates in a pi_world's simulation.
// ================================================================================================
class pi_body
{
public:
    struct create_params
    {
        // These body values are immutable once the body is created, if you want to change them,
        // then the body should be recreated.

        // ID of collision type that defines what other bodies this body interacts with.
        string_hash collision_type;

        // Shape of the collision for this body.
        pi_shape shape;

        // If true this body is considered to be moving, otherwise it will be considered
        // to be static within the scene. Static applies various optimizations, so it should
        // be used where possible.
        bool dynamic;
    };

public:
    virtual ~pi_body() {}

    // Gets the name this body was created with.
    virtual const char* get_debug_name() = 0;

    // If settings are changed during simulation, the result may not be reflected until after 
    // the simulation finishes.

    // Gets or sets the transform of a body. 
    virtual void set_transform(const vector3& location, const quat& rotation) = 0;
    virtual void get_transform(vector3& location, quat& rotation) = 0;

    // Gets or sets the current linear velocity (in units/s).
    virtual vector3 get_linear_velocity() = 0;
    virtual void set_linear_velocity(const vector3& new_velocity) = 0;

    // Gets or sets the current angular velocity (in radians/s).
    virtual vector3 get_angular_velocity() = 0;
    virtual void set_angular_velocity(const vector3& new_velocity) = 0;


    // Adds force in newtons at the given point on the body in world space.
    virtual void add_force_at_point(const vector3& force, const vector3& position) = 0;

    // Adds force in newtons at the center of mass.
    virtual void add_force(const vector3& force) = 0;

    // Adds torque in newtons meters.
    virtual void add_torque(const vector3& torque) = 0;


    // Adds impulse in kg unit/s at the given point on the body in world space.
    virtual void add_impulse_at_point(const vector3& force, const vector3& position) = 0;

    // Adds impulse in kg unit/s at the center of mass.
    virtual void add_impulse(const vector3& force) = 0;

    // Adds torque in newton unit/s.
    virtual void add_angular_impulse(const vector3& torque) = 0;


    // Returns true if the body is awake and actively being simulated, returns false when resting.
    virtual bool is_awake() = 0;

};

}; // namespace ws
