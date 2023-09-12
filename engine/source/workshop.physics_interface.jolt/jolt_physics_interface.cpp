// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.physics_interface.jolt/jolt_physics_interface.h"
#include "workshop.core/perf/profile.h"

#include "thirdparty/JoltPhysics/Jolt/Jolt.h"

namespace ws {

jolt_physics_interface::jolt_physics_interface()
{
}

void jolt_physics_interface::register_init(init_list& list)
{
    list.add_step(
        "Jolt Physics",
        [this, &list]() -> result<void> { return create_jolt(list); },
        [this]() -> result<void> { return destroy_jolt(); }
    );
}

result<void> jolt_physics_interface::create_jolt(init_list& list)
{
    return true;
}

result<void> jolt_physics_interface::destroy_jolt()
{
    return true;
}

}; // namespace ws
