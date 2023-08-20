// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/system.h"

namespace ws {

// ================================================================================================
//  Moves all objects with a fly_camera_component with very simple wasd/mouse movement.
// ================================================================================================
class fly_camera_movement_system : public system
{
public:

    fly_camera_movement_system(object_manager& manager);

    virtual void step(const frame_time& time) override;

};

}; // namespace ws
