// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/component.h"

#include "workshop.core/math/quat.h"
#include "workshop.core/math/vector3.h"

namespace ws {

// ================================================================================================
//  The fly camera provides simple movement of the camera view with wasd/mouse.
// ================================================================================================
class fly_camera_movement_component : public component
{
public:

    // How much the mouse delta is scaled to determine angular speed.
    float sensitivity = 0.001f;

    // Speed of camera movement in units per second.
    float speed = 1500.0f;

};

}; // namespace ws
