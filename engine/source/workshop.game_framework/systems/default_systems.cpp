// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/default_systems.h"
#include "workshop.game_framework/systems/camera/fly_camera_movement_system.h"
#include "workshop.game_framework/systems/camera/camera_system.h"
#include "workshop.game_framework/systems/lighting/directional_light_system.h"
#include "workshop.game_framework/systems/lighting/point_light_system.h"
#include "workshop.game_framework/systems/lighting/spot_light_system.h"
#include "workshop.game_framework/systems/lighting/light_probe_grid_system.h"
#include "workshop.game_framework/systems/lighting/reflection_probe_system.h"
#include "workshop.game_framework/systems/geometry/static_mesh_system.h"
#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.engine/ecs/object_manager.h"

namespace ws {

void register_default_systems(object_manager& manager)
{
    manager.register_system<transform_system>();

    manager.register_system<camera_system>();
    manager.register_system<directional_light_system>();
    manager.register_system<point_light_system>();
    manager.register_system<spot_light_system>();;
    manager.register_system<light_probe_grid_system>();
    manager.register_system<reflection_probe_system>();
    manager.register_system<static_mesh_system>();

    manager.register_system<fly_camera_movement_system>();
}

}; // namespace ws
