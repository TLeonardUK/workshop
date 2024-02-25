// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.physics_interface/pi_world.h"
#include "workshop.physics_interface/physics_cvars.h"
#include "workshop.core/utils/result.h"
#include "workshop.core/utils/frame_time.h"

#include <string>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

namespace ws {

class jolt_pi_interface;

// ================================================================================================
//  Implementation of a physics world in jolt.
// ================================================================================================
class jolt_pi_world : public pi_world
{
public:
    jolt_pi_world(jolt_pi_interface& renderer, const pi_world::create_params& params, const char* debug_name);
    virtual ~jolt_pi_world();

    result<void> create_resources();

    virtual const char* get_debug_name() override;
    
    virtual void step(const frame_time& time) override;

    virtual std::unique_ptr<pi_body> create_body(const pi_body::create_params& create_params, const char* name) override;
    virtual void add_body(pi_body& body) override;
    virtual void remove_body(pi_body& body) override;

public:

    const pi_collision_type& get_layer_collision_type(size_t layer_index);
    size_t get_layer_count();

    // Gets an object layer from the given collision type.
    JPH::ObjectLayer get_object_layer(const string_hash& collision_type_id);

    JPH::PhysicsSystem& get_physics_system();

private:        
    jolt_pi_interface& m_interface;
    std::string m_debug_name;
    pi_world::create_params m_create_params;

    std::unique_ptr<JPH::BroadPhaseLayerInterface> m_bp_layer_filter;
    std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilter> m_object_vs_bp_layer_filter;
    std::unique_ptr<JPH::ObjectLayerPairFilter> m_object_layer_pair_filter;

    std::unique_ptr<JPH::PhysicsSystem> m_physics_system;

    std::vector<pi_collision_type> m_collision_types;
 
};

}; // namespace ws
