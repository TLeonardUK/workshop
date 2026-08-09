// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.physics_interface.stub/stub_pi_interface.h"
#include "workshop.physics_interface.stub/stub_pi_world.h"

namespace ws {

stub_pi_interface::stub_pi_interface()
{
}

void stub_pi_interface::register_init(init_list& list)
{
}

std::unique_ptr<pi_world> stub_pi_interface::create_world(const pi_world::create_params& params, const char* debug_name)
{
    return std::make_unique<stub_pi_world>(params, debug_name);
}

}; // namespace ws
