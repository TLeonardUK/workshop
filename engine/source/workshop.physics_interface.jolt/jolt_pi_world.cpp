// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.physics_interface.jolt/jolt_pi_world.h"
#include "workshop.physics_interface.jolt/jolt_pi_interface.h"
#include "workshop.physics_interface.jolt/jolt_pi_body.h"

#include "workshop.core/perf/profile.h"

namespace ws {

class jolt_pi_object_layer_pair_filter : public JPH::ObjectLayerPairFilter
{
public:
    jolt_pi_object_layer_pair_filter(jolt_pi_world& world)
        : m_world(world)
    {
    }

	virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
	{
        const pi_collision_type& layer_1_type = m_world.get_layer_collision_type((size_t)inObject1);
        const pi_collision_type& layer_2_type = m_world.get_layer_collision_type((size_t)inObject2);

        for (const string_hash& colliding_type : layer_1_type.collides_with)
        {
            if (colliding_type == layer_2_type.id)
            {
                return true;
            }
        }

        return false;
	}

private:
    jolt_pi_world& m_world;
};

class jolt_pi_broadphase_layer_filter final : public JPH::BroadPhaseLayerInterface
{
public:
    jolt_pi_broadphase_layer_filter(jolt_pi_world& world)
        : m_world(world)
    {
    }
    
	virtual JPH::uint GetNumBroadPhaseLayers() const override
	{
		return (JPH::uint)m_world.get_layer_count();
	}

	virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
	{
        // We have an identical mapping of broadphase -> object layer at the moment.
        return JPH::BroadPhaseLayer((JPH::BroadPhaseLayer::Type)inLayer);
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
	{
        const pi_collision_type& layer_1_type = m_world.get_layer_collision_type((JPH::BroadPhaseLayer::Type)inLayer);
        return layer_1_type.id.c_str();
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
    jolt_pi_world& m_world;
};

class jolt_pi_object_vs_broadphase_layer_filter : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    jolt_pi_object_vs_broadphase_layer_filter(jolt_pi_world& world)
        : m_world(world)
    {
    }
	
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
	{
        const pi_collision_type& layer_1_type = m_world.get_layer_collision_type((size_t)inLayer1);
        const pi_collision_type& layer_2_type = m_world.get_layer_collision_type((JPH::BroadPhaseLayer::Type)inLayer2);

        for (const string_hash& colliding_type : layer_1_type.collides_with)
        {
            if (colliding_type == layer_2_type.id)
            {
                return true;
            }
        }

        return false;
	}

private:
    jolt_pi_world& m_world;
};

jolt_pi_world::jolt_pi_world(jolt_pi_interface& in_interface, const pi_world::create_params& params, const char* debug_name)
    : m_interface(in_interface)
    , m_debug_name(debug_name)
    , m_create_params(params)
    , m_collision_types(params.collision_types)
{
    db_assert(!m_collision_types.empty());
}

jolt_pi_world::~jolt_pi_world()
{
}

result<void> jolt_pi_world::create_resources()
{
    m_bp_layer_filter = std::make_unique<jolt_pi_broadphase_layer_filter>(*this);
    m_object_vs_bp_layer_filter = std::make_unique<jolt_pi_object_vs_broadphase_layer_filter>(*this);
    m_object_layer_pair_filter = std::make_unique<jolt_pi_object_layer_pair_filter>(*this);

    m_physics_system = std::make_unique<JPH::PhysicsSystem>();
    m_physics_system->Init(
        cvar_physics_max_bodies.get(), 
        0, 
        cvar_physics_max_bodies.get(),
        cvar_physics_max_constraints.get(),
        *m_bp_layer_filter,
        *m_object_vs_bp_layer_filter,
        *m_object_layer_pair_filter
    );

    return true;
}

const char* jolt_pi_world::get_debug_name()
{
    return m_debug_name.c_str();
}

void jolt_pi_world::step(const frame_time& time)
{
    m_physics_system->Update(
        time.delta_seconds,
        cvar_physics_collision_steps.get_int(),
        cvar_physics_integration_steps.get_int(),
        &m_interface.get_temp_allocator(),
        &m_interface.get_job_system()
    );
}

const pi_collision_type& jolt_pi_world::get_layer_collision_type(size_t layer_index)
{
    return m_collision_types[layer_index];
}

size_t jolt_pi_world::get_layer_count()
{
    return m_collision_types.size();
}

JPH::ObjectLayer jolt_pi_world::get_object_layer(const string_hash& collision_type_id)
{
    for (size_t i = 0; i < m_collision_types.size(); i++)
    {
        if (m_collision_types[i].id == collision_type_id)
        {
            return JPH::ObjectLayer(i);
        }
    }

    return 0;
}

JPH::PhysicsSystem& jolt_pi_world::get_physics_system()
{
    return *m_physics_system.get();
}

std::unique_ptr<pi_body> jolt_pi_world::create_body(const pi_body::create_params& create_params, const char* debug_name)
{
    std::unique_ptr<jolt_pi_body> instance = std::make_unique<jolt_pi_body>(*this, create_params, debug_name);
    if (!instance->create_resources())
    {
        return nullptr;
    }
    return instance;
}

void jolt_pi_world::add_body(pi_body& body)
{
    jolt_pi_body& jolt_body = static_cast<jolt_pi_body&>(body);

    m_physics_system->GetBodyInterface().AddBody(
        jolt_body.get_body_id(),
        JPH::EActivation::Activate
    );
}

void jolt_pi_world::remove_body(pi_body& body)
{
    jolt_pi_body& jolt_body = static_cast<jolt_pi_body&>(body);

    m_physics_system->GetBodyInterface().RemoveBody(
        jolt_body.get_body_id()
    );
}

}; // namespace ws
