// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/system.h"
#include "workshop.core/math/vector2.h"
#include "workshop.core/math/rect.h"

namespace ws {

// ================================================================================================
//  Moves all objects with a editor_camera_component with very simple wasd/mouse movement.
// ================================================================================================
class editor_camera_movement_system : public system
{
public:

    editor_camera_movement_system(object_manager& manager);

    virtual void step(const frame_time& time) override;

public:

    // Public Commands

    void set_input_state(object handle, const recti& input_viewport, bool mouse_over, bool input_blocked);

private:    

    // State of mouse last frame.
    bool m_mouse_down_last = false;

    // How much we have to move the mouse after first pressing button
    // before capturing the mouse.
    constexpr inline static float k_movement_capture_threshold = 8.0f;

};

}; // namespace ws
