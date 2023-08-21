// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/camera/camera_system.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/camera/camera_component.h"
#include "workshop.game_framework/systems/transform/transform_system.h"

namespace ws {

camera_system::camera_system(object_manager& manager)
    : system(manager, "camera system")
{
    add_predecessor<transform_system>();
}

void camera_system::step(const frame_time& time)
{
    component_filter<transform_component, camera_component> filter(m_manager);
    for (size_t i = 0; i < filter.size(); i++)
    {
        object obj = filter.get_object(i);

        transform_component* transform = filter.get_component<transform_component>(i);
        camera_component* camera = filter.get_component<camera_component>(i);

    }

    // todo: remove old component views.
}

}; // namespace ws
