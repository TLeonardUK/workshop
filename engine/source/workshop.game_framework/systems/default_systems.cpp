// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/default_systems.h"
#include "workshop.game_framework/systems/camera/fly_camera_movement_system.h"
#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.engine/ecs/object_manager.h"

namespace ws {

void register_default_systems(object_manager& manager)
{
    manager.register_system<transform_system>();

    manager.register_system<fly_camera_movement_system>();
}

}; // namespace ws
