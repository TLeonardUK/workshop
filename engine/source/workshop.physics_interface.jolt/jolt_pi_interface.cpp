// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.physics_interface.jolt/jolt_pi_interface.h"
#include "workshop.physics_interface.jolt/jolt_pi_world.h"
#include "workshop.core/perf/profile.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>

namespace ws {

jolt_pi_interface::jolt_pi_interface()
{
}

JPH::TempAllocatorImpl& jolt_pi_interface::get_temp_allocator()
{
    return *m_temp_allocator;
}

jolt_pi_job_system& jolt_pi_interface::get_job_system()
{
    return m_job_system;
}

void jolt_pi_interface::register_init(init_list& list)
{
    list.add_step(
        "Jolt Physics",
        [this, &list]() -> result<void> { return create_jolt(list); },
        [this]() -> result<void> { return destroy_jolt(); }
    );
}

result<void> jolt_pi_interface::create_jolt(init_list& list)
{
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    m_temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(32*1024*1024);

    return true;
}

result<void> jolt_pi_interface::destroy_jolt()
{
    m_temp_allocator = nullptr;

    JPH::UnregisterTypes();

    return true;
}

std::unique_ptr<pi_world> jolt_pi_interface::create_world(const char* debug_name)
{
    std::unique_ptr<jolt_pi_world> instance = std::make_unique<jolt_pi_world>(*this, debug_name);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

}; // namespace ws
