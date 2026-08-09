// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.physics_interface/physics_interface.h"

namespace ws {

// ================================================================================================
//  Stub implementation of physics_interface, performs no actual simulation. Useful for headless
//  builds or platforms that do not have a real physics implementation available.
// ================================================================================================
class stub_pi_interface : public physics_interface
{
public:
    stub_pi_interface();

    virtual void register_init(init_list& list) override;

    virtual std::unique_ptr<pi_world> create_world(const pi_world::create_params& params, const char* debug_name = nullptr) override;

};

}; // namespace ws
