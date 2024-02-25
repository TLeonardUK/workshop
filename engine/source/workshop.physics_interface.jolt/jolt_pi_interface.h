// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.physics_interface/physics_interface.h"

#include "workshop.physics_interface.jolt/jolt_pi_job_system.h"

#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>

namespace ws {

// ================================================================================================
//  Implementation of input using the Jolt library.
// ================================================================================================
class jolt_pi_interface : public physics_interface
{
public:
    jolt_pi_interface();

    virtual void register_init(init_list& list) override;

    virtual std::unique_ptr<pi_world> create_world(const pi_world::create_params& params, const char* debug_name = nullptr) override;

    JPH::TempAllocatorImpl& get_temp_allocator();
    jolt_pi_job_system& get_job_system();

protected:

    result<void> create_jolt(init_list& list);
    result<void> destroy_jolt();

private:
    std::unique_ptr<JPH::TempAllocatorImpl> m_temp_allocator;
    std::unique_ptr<jolt_pi_job_system> m_job_system;
    

};

}; // namespace ws
