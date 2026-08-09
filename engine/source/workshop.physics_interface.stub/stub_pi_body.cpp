// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.physics_interface.stub/stub_pi_body.h"

namespace ws {

stub_pi_body::stub_pi_body(const create_params& params, const char* debug_name)
    : m_params(params)
    , m_debug_name(debug_name ? debug_name : "")
{
}

const char* stub_pi_body::get_debug_name()
{
    return m_debug_name.c_str();
}

void stub_pi_body::set_transform(const vector3& location, const quat& rotation)
{
    m_location = location;
    m_rotation = rotation;
}

void stub_pi_body::get_transform(vector3& location, quat& rotation)
{
    location = m_location;
    rotation = m_rotation;
}

vector3 stub_pi_body::get_linear_velocity()
{
    return m_linear_velocity;
}

void stub_pi_body::set_linear_velocity(const vector3& new_velocity)
{
    m_linear_velocity = new_velocity;
}

vector3 stub_pi_body::get_angular_velocity()
{
    return m_angular_velocity;
}

void stub_pi_body::set_angular_velocity(const vector3& new_velocity)
{
    m_angular_velocity = new_velocity;
}

void stub_pi_body::add_force_at_point(const vector3& force, const vector3& position)
{
}

void stub_pi_body::add_force(const vector3& force)
{
}

void stub_pi_body::add_torque(const vector3& torque)
{
}

void stub_pi_body::add_impulse_at_point(const vector3& force, const vector3& position)
{
}

void stub_pi_body::add_impulse(const vector3& force)
{
}

void stub_pi_body::add_angular_impulse(const vector3& torque)
{
}

bool stub_pi_body::is_awake()
{
    return false;
}

}; // namespace ws
