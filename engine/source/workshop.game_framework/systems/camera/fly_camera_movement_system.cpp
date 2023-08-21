// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/camera/fly_camera_movement_system.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/camera/camera_component.h"
#include "workshop.game_framework/components/camera/fly_camera_movement_component.h"

namespace ws {

fly_camera_movement_system::fly_camera_movement_system(object_manager& manager)
    : system(manager, "fly camera movement system")
{
}

void fly_camera_movement_system::step(const frame_time& time)
{
    component_filter<transform_component, camera_component, fly_camera_movement_component> filter(m_manager);
    for (size_t i = 0; i < filter.size(); i++)
    {
        object obj = filter.get_object(i);

        transform_component* transform = filter.get_component<transform_component>(i);
        camera_component* camera = filter.get_component<camera_component>(i);
        fly_camera_movement_component* movement = filter.get_component<fly_camera_movement_component>(i);
        
        // do stuff
    }
}

}; // namespace ws
