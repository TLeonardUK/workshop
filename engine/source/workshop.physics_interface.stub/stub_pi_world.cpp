// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.physics_interface.stub/stub_pi_world.h"
#include "workshop.physics_interface.stub/stub_pi_body.h"

namespace ws {

stub_pi_world::stub_pi_world(const create_params& params, const char* debug_name)
    : m_params(params)
    , m_debug_name(debug_name ? debug_name : "")
{
}

const char* stub_pi_world::get_debug_name()
{
    return m_debug_name.c_str();
}

void stub_pi_world::step(const frame_time& time)
{
}

std::unique_ptr<pi_body> stub_pi_world::create_body(const pi_body::create_params& create_params, const char* name)
{
    return std::make_unique<stub_pi_body>(create_params, name);
}

void stub_pi_world::add_body(pi_body& body)
{
}

void stub_pi_world::remove_body(pi_body& body)
{
}

}; // namespace ws
