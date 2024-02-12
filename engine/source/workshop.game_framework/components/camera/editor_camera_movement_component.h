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
//  The editor camera provides camera movement in an editor viewport
// ================================================================================================
class editor_camera_movement_component : public component
{
public:

    // How much the mouse delta is scaled to determine angular speed.
    float sensitivity = 0.001f;

    // Speed of camera movement in units per second.
    float speed = 1500.0f;

    // Speed of camera movement for each mouse wheel rotation.
    float zoom_speed = 50000.0f;

    // Speed of camera movement when panning with uncaptured mouse movement.
    float pan_speed = 50.0f;

    // Determines the maximum vertical angle of the camera to avoid the camera
    // looping around on its rotations.
    // 
    // This is represented as a dot product value.
    // 1 allows the camera to go fully vertical, 0.5 allows a max 45 degree angle, etc.
    float max_vertical_angle = 0.8f;

    // If this camera is focused and recieving input.
    bool is_focused = false;

private:

    friend class editor_camera_movement_system;

    // The viewport in screen coordinates that this camera should take input from. This will
    // match up to an editor viewport being rendered.
    recti input_viewport = recti::empty;

    // If the mouse is over the viewport.
    bool input_mouse_over = false;

    // If true all movement input is blocked.
    bool input_blocked = false;

    // Number of frames this camera has been focused for.
    size_t focused_frames = 0;

    // Number of frames camera has been focused and mouse has been down.
    size_t focused_down_frames = 0;

    // Location that the mouse starts being pressed down.
    vector2 start_mouse_down_position = vector2::zero;

    // Rotation we want to apply to camera in euler coordinates.
    // TODO: Remove this and do it statelessly.
    vector3 rotation_euler = vector3::zero;
    
private:    
    BEGIN_REFLECT(editor_camera_movement_component, "Editor Camera Movement", component, reflect_class_flags::none)
    END_REFLECT()

};

}; // namespace ws
