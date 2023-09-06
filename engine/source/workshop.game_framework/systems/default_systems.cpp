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
#include "workshop.game_framework/systems/transform/bounds_system.h"

#include "workshop.game_framework/components/camera/camera_component.h"
#include "workshop.game_framework/components/camera/fly_camera_movement_component.h"
#include "workshop.game_framework/components/geometry/static_mesh_component.h"
#include "workshop.game_framework/components/lighting/directional_light_component.h"
#include "workshop.game_framework/components/lighting/light_probe_grid_component.h"
#include "workshop.game_framework/components/lighting/point_light_component.h"
#include "workshop.game_framework/components/lighting/reflection_probe_component.h"
#include "workshop.game_framework/components/lighting/spot_light_component.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/transform/bounds_component.h"

#include "workshop.engine/ecs/object_manager.h"
#include "workshop.engine/ecs/meta_component.h"

namespace ws {

void register_default_systems(object_manager& manager)
{
    // Components

    manager.register_component<meta_component>();

    manager.register_component<transform_component>();
    manager.register_component<bounds_component>();

    manager.register_component<directional_light_component>();
    manager.register_component<light_probe_grid_component>();
    manager.register_component<point_light_component>();
    manager.register_component<reflection_probe_component>();
    manager.register_component<spot_light_component>();

    manager.register_component<static_mesh_component>();

    manager.register_component<camera_component>();
    manager.register_component<fly_camera_movement_component>();

    // Systems

    manager.register_system<transform_system>();
    manager.register_system<bounds_system>();

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
