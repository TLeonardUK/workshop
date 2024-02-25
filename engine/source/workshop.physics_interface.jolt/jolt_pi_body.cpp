// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.physics_interface.jolt/jolt_pi_body.h"
#include "workshop.physics_interface.jolt/jolt_pi_world.h"
#include "workshop.physics_interface.jolt/jolt_pi_interface.h"

#include "workshop.core/perf/profile.h"


#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

// Workshop works in cm units, jolt works in meter units.
#define WS_TO_JOLT_UNIT_SCALE 0.01f
#define JOLT_TO_WS_UNIT_SCALE 100.0f

namespace ws {

jolt_pi_body::jolt_pi_body(jolt_pi_world& world, const pi_body::create_params& create_params, const char* debug_name)
    : m_world(world)
    , m_debug_name(debug_name)
    , m_create_params(create_params)
{
}

jolt_pi_body::~jolt_pi_body()
{
    auto& body_interface = m_world.get_physics_system().GetBodyInterface();
    body_interface.RemoveBody(get_body_id());
    body_interface.DestroyBody(get_body_id());

    m_body = nullptr;
}

result<void> jolt_pi_body::create_resources()
{
    switch (m_create_params.shape.shape)
    {
        case pi_shape::type::box:
        {
            JPH::BoxShapeSettings settings(
                JPH::Vec3Arg(
                    (m_create_params.shape.extents.x * 0.5f) * WS_TO_JOLT_UNIT_SCALE,
                    (m_create_params.shape.extents.y * 0.5f) * WS_TO_JOLT_UNIT_SCALE,
                    (m_create_params.shape.extents.z * 0.5f) * WS_TO_JOLT_UNIT_SCALE
                )
            );

            m_shape = settings.Create();

            break;
        }
        case pi_shape::type::capsule:
        {
            JPH::CapsuleShapeSettings settings(
                (m_create_params.shape.height * 0.5f) * WS_TO_JOLT_UNIT_SCALE,
                m_create_params.shape.radius * WS_TO_JOLT_UNIT_SCALE
            );

            m_shape = settings.Create();

            break;
        }
        case pi_shape::type::sphere:
        {
            JPH::SphereShapeSettings settings(
                (m_create_params.shape.radius) * WS_TO_JOLT_UNIT_SCALE
            );

            m_shape = settings.Create();

            break;
        }
    }

    JPH::BodyCreationSettings creation_settings(
        m_shape.Get(),
        JPH::RVec3(0.0f, 0.0f, 0.0f),
        JPH::Quat::sIdentity(),
        m_create_params.dynamic ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static,
        m_world.get_object_layer(m_create_params.collision_type)
    );

    m_body = m_world.get_physics_system().GetBodyInterface().CreateBody(
        creation_settings
    );

    if (!m_body)
    {
        return false;
    }

    return true;
}

const char* jolt_pi_body::get_debug_name()
{
    return m_debug_name.c_str();
}

void jolt_pi_body::set_transform(const vector3& location, const quat& rotation)
{
    m_world.get_physics_system().GetBodyInterface().SetPositionAndRotation(
        get_body_id(),
        JPH::RVec3Arg(location.x * WS_TO_JOLT_UNIT_SCALE, location.y * WS_TO_JOLT_UNIT_SCALE, location.z * WS_TO_JOLT_UNIT_SCALE),
        JPH::QuatArg(rotation.x, rotation.y, rotation.z, rotation.w),
        JPH::EActivation::Activate
    );
}

void jolt_pi_body::get_transform(vector3& location, quat& rotation)
{
    JPH::RVec3 pos = m_body->GetPosition();
    JPH::Quat rot = m_body->GetRotation();
    location = vector3(pos.GetX(), pos.GetY(), pos.GetZ()) * JOLT_TO_WS_UNIT_SCALE;
    rotation = quat(rot.GetX(), rot.GetY(), rot.GetZ(), rot.GetW());
}

vector3 jolt_pi_body::get_linear_velocity()
{
    JPH::Vec3 vel = m_body->GetLinearVelocity();
    return vector3(vel.GetX(), vel.GetX(), vel.GetZ()) * JOLT_TO_WS_UNIT_SCALE;
}

void jolt_pi_body::set_linear_velocity(const vector3& new_velocity)
{
    m_body->SetLinearVelocity(JPH::Vec3Arg(
        new_velocity.x * WS_TO_JOLT_UNIT_SCALE,
        new_velocity.y * WS_TO_JOLT_UNIT_SCALE,
        new_velocity.z * WS_TO_JOLT_UNIT_SCALE
    ));
}

vector3 jolt_pi_body::get_angular_velocity()
{
    JPH::Vec3 vel = m_body->GetAngularVelocity();
    return vector3(vel.GetX(), vel.GetX(), vel.GetZ());
}

void jolt_pi_body::set_angular_velocity(const vector3& new_velocity)
{
    m_body->SetAngularVelocity(JPH::Vec3Arg(
        new_velocity.x,
        new_velocity.y,
        new_velocity.z
    ));
}

void jolt_pi_body::add_force_at_point(const vector3& force, const vector3& position)
{
    m_body->AddForce(
        JPH::Vec3Arg(force.x, force.y, force.z),
        JPH::RVec3Arg(position.x * WS_TO_JOLT_UNIT_SCALE, position.y * WS_TO_JOLT_UNIT_SCALE, position.z * WS_TO_JOLT_UNIT_SCALE)
    );
}

void jolt_pi_body::add_force(const vector3& force)
{
    m_body->AddForce(
        JPH::Vec3Arg(force.x, force.y, force.z)
    );
}

void jolt_pi_body::add_torque(const vector3& torque)
{
    m_body->AddTorque(
        JPH::Vec3Arg(torque.x, torque.y, torque.z)
    );
}

void jolt_pi_body::add_impulse_at_point(const vector3& force, const vector3& position)
{
    m_body->AddImpulse(
        JPH::Vec3Arg(force.x * WS_TO_JOLT_UNIT_SCALE, force.y * WS_TO_JOLT_UNIT_SCALE, force.z * WS_TO_JOLT_UNIT_SCALE),
        JPH::RVec3Arg(position.x * WS_TO_JOLT_UNIT_SCALE, position.y * WS_TO_JOLT_UNIT_SCALE, position.z * WS_TO_JOLT_UNIT_SCALE)
    );
}

void jolt_pi_body::add_impulse(const vector3& force)
{
    m_body->AddImpulse(
        JPH::Vec3Arg(force.x * WS_TO_JOLT_UNIT_SCALE, force.y * WS_TO_JOLT_UNIT_SCALE, force.z * WS_TO_JOLT_UNIT_SCALE)
    );
}

void jolt_pi_body::add_angular_impulse(const vector3& force)
{
    m_body->AddAngularImpulse(
        JPH::Vec3Arg(force.x, force.y, force.z)
    );
}

bool jolt_pi_body::is_awake()
{
    return m_body->IsActive();
}

JPH::BodyID jolt_pi_body::get_body_id()
{
    return m_body->GetID();
}

}; // namespace ws
