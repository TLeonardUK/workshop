// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.physics_interface/pi_body.h"

#include <string>

namespace ws {

// ================================================================================================
//  Stub implementation of a rigid body, performs no actual simulation.
// ================================================================================================
class stub_pi_body : public pi_body
{
public:
    stub_pi_body(const create_params& params, const char* debug_name);

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

private:
    create_params m_params;
    std::string m_debug_name;

    vector3 m_location;
    quat m_rotation;

    vector3 m_linear_velocity;
    vector3 m_angular_velocity;

};

}; // namespace ws
