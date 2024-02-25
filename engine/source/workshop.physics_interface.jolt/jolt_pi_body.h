// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.physics_interface/pi_world.h"
#include "workshop.physics_interface/pi_body.h"
#include "workshop.core/utils/result.h"
#include "workshop.core/utils/frame_time.h"

#include <string>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

namespace ws {

class jolt_pi_interface;
class jolt_pi_world;

// ================================================================================================
//  Implementation a rigid body using jolt.
// ================================================================================================
class jolt_pi_body : public pi_body
{
public:
    jolt_pi_body(jolt_pi_world& world, const pi_body::create_params& create_params, const char* debug_name);
    virtual ~jolt_pi_body();

    result<void> create_resources();

    virtual const char* get_debug_name() override;
    
    virtual void set_transform(const vector3& location, const quat& rotation) override;
    virtual void get_transform(vector3& location, quat& rotation) override;
    virtual vector3 get_linear_velocity() override;
    virtual void set_linear_velocity(const vector3& new_velocity) override;
    virtual vector3 get_angular_velocity() override;
    virtual void set_angular_velocity(const vector3& new_velocity) override;

    virtual void add_force_at_point(const vector3& force, const vector3& position) override;
    virtual void add_force(const vector3& force) override;
    virtual void add_torque(const vector3& torque) override;

    virtual void add_impulse_at_point(const vector3& force, const vector3& position) override;
    virtual void add_impulse(const vector3& force) override;
    virtual void add_angular_impulse(const vector3& torque) override;

    virtual bool is_awake() override;

    JPH::BodyID get_body_id();

private:        
    jolt_pi_world& m_world;
    std::string m_debug_name;
    pi_body::create_params m_create_params;

    JPH::Body* m_body = nullptr;
    JPH::ShapeSettings::ShapeResult m_shape;

};

}; // namespace ws
