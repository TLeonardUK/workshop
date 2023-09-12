// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/system.h"
#include "workshop.core/math/vector2.h"

namespace ws {

// ================================================================================================
//  Moves all objects with a fly_camera_component with very simple wasd/mouse movement.
// ================================================================================================
class fly_camera_movement_system : public system
{
public:

    fly_camera_movement_system(object_manager& manager);

    virtual void step(const frame_time& time) override;

private:
    size_t m_uncaptured_mouse_input_delay = 0;
    size_t m_viewport_control_frames = 0;

    bool m_last_mouse_down = false;
    bool m_mouse_down_started_over_viewport = false;
    vector2 m_mouse_down_start_location = vector2::zero;

    // How much we have to move the mouse after first pressing button
    // before capturing the mouse.
    constexpr inline static float k_movement_capture_threshold = 8.0f;

};

}; // namespace ws
