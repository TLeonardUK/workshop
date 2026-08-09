// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.physics_interface/pi_world.h"

#include <string>

namespace ws {

// ================================================================================================
//  Stub implementation of a physics world, performs no actual simulation.
// ================================================================================================
class stub_pi_world : public pi_world
{
public:
    stub_pi_world(const create_params& params, const char* debug_name);

    virtual const char* get_debug_name() override;

    virtual void step(const frame_time& time) override;

    virtual std::unique_ptr<pi_body> create_body(const pi_body::create_params& create_params, const char* name) override;

    virtual void add_body(pi_body& body) override;
    virtual void remove_body(pi_body& body) override;

private:
    create_params m_params;
    std::string m_debug_name;

};

}; // namespace ws
