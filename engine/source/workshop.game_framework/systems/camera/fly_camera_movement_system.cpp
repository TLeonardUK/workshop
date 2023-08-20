// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/camera/fly_camera_movement_system.h"

namespace ws {

fly_camera_movement_system::fly_camera_movement_system(object_manager& manager)
    : system(manager, "fly camera movement system")
{
}

void fly_camera_movement_system::step(const frame_time& time)
{
    // TODO
}

}; // namespace ws
