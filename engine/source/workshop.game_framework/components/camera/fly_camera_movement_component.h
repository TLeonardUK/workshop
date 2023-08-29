// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/component.h"

#include "workshop.core/math/quat.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/reflection/reflect.h"

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

    // Determines the maximum vertical angle of the camera to avoid the camera
    // looping around on its rotations.
    // 
    // This is represented as a dot product value.
    // 1 allows the camera to go fully vertical, 0.5 allows a max 45 degree angle, etc.
    float max_vertical_angle = 0.8f;

private:

    friend class fly_camera_movement_system;

    // Number of frame the mouse has been captured
    size_t mouse_capture_frames = 0;

    // Rotation we want to apply to camera in euler coordinates.
    // TODO: Remove this and do it statelessly.
    vector3 rotation_euler = vector3::zero;
    
public:

    BEGIN_REFLECT(fly_camera_movement_component, "Fly Camera Movement", component, reflect_class_flags::none)
        REFLECT_FIELD(sensitivity,              "Sensitivity",              "How much the mouse delta is scaled to determine angular speed.")
        REFLECT_FIELD(speed,                    "Speed",                    "Speed of camera movement in units per second.")
        REFLECT_FIELD(max_vertical_angle,       "Max Vertical Angle",       "Determines the maximum vertical angle of the camera to avoid the camera looping around on its rotations.\n\nThis is represented as a dot product value.\n1 allows the camera to go fully vertical, 0.5 allows a max 45 degree angle, etc.")

        REFLECT_CONSTRAINT_RANGE(sensitivity,           0.0f, 100000.0f)
        REFLECT_CONSTRAINT_RANGE(speed,                 0.0f, 100000.0f)
        REFLECT_CONSTRAINT_RANGE(max_vertical_angle,    0.0f, 1.0f)
    END_REFLECT()

};

}; // namespace ws
