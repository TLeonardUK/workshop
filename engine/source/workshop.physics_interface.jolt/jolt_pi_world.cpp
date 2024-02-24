// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.physics_interface.jolt/jolt_pi_world.h"
#include "workshop.physics_interface.jolt/jolt_pi_interface.h"

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
	virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
	{
        const pi_collision_type& layer_1_type = m_world.get_layer_collision_type((JPH::BroadPhaseLayer::Type)iinLayer);
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

jolt_pi_world::jolt_pi_world(jolt_pi_interface& in_interface, const char* debug_name)
    : m_interface(in_interface)
    , m_debug_name(debug_name)
{
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

void jolt_pi_world::set_collision_types(const std::vector<pi_collision_type>& types)
{
    m_collision_types = types;

    // TODO: Validate all bodies have valid collision types.
}

const pi_collision_type& jolt_pi_world::get_layer_collision_type(size_t layer_index)
{
    return m_collision_types[layer_index];
}

size_t jolt_pi_world::get_layer_count()
{
    return m_collision_types.size();
}

}; // namespace ws
