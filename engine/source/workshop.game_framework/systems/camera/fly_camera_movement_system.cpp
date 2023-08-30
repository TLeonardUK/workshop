// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/camera/fly_camera_movement_system.h"
#include "workshop.game_framework/systems/camera/camera_system.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/camera/camera_component.h"
#include "workshop.game_framework/components/camera/fly_camera_movement_component.h"
#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.input_interface/input_interface.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.renderer/renderer.h"

#pragma optimize("", off)

namespace ws {

fly_camera_movement_system::fly_camera_movement_system(object_manager& manager)
    : system(manager, "fly camera movement system")
{
    // We want to apply any movement before the transform or camera system
    // so they have the most up to date transforms for this frame.
    add_successor<transform_system>();
    add_successor<camera_system>();
}

void fly_camera_movement_system::step(const frame_time& time)
{
    engine& engine = m_manager.get_world().get_engine();
    input_interface& input = engine.get_input_interface();
    vector2 screen_size = vector2(
        (float)engine.get_renderer().get_display_width(),
        (float)engine.get_renderer().get_display_height()
    );

    bool w_down = input.is_key_down(input_key::w);
    bool s_down = input.is_key_down(input_key::s);
    bool a_down = input.is_key_down(input_key::a);
    bool d_down = input.is_key_down(input_key::d);
    bool q_down = input.is_key_down(input_key::q);
    bool e_down = input.is_key_down(input_key::e);
    bool mouse_captured = input.get_mouse_capture();
    bool mouse_over_viewport = engine.get_mouse_over_viewport();
    float mouse_delta = input.get_mouse_wheel_delta(false);

    bool lmb_down = input.is_key_down(input_key::mouse_left);
    bool rmb_down = input.is_key_down(input_key::mouse_right);

    float forward_movement = (static_cast<float>(w_down) - static_cast<float>(s_down)) * time.delta_seconds;
    float right_movement = (static_cast<float>(d_down) - static_cast<float>(a_down)) * time.delta_seconds;
    float up_movement = (static_cast<float>(e_down) - static_cast<float>(q_down)) * time.delta_seconds;
    float mouse_delta_movement = mouse_delta * time.delta_seconds;

    plane y_plane = plane(vector3::up, vector3::zero);
    float y_plane_movement = 0.0f;

    float pan_right_movement = 0.0f;
    float pan_up_movement = 0.0f;

    if ((lmb_down || rmb_down) && mouse_over_viewport)
    {
        m_viewport_control_frames++;
    }
    else
    {
        m_viewport_control_frames = 0;
    }

    // Calculate how much the mouse has moved from the center of the screen and reset
    // it to the center.
    vector2 center_pos = screen_size * 0.5f;
    vector2 delta_pos = (input.get_mouse_position() - center_pos);
    bool reset_mouse = mouse_captured || m_viewport_control_frames > 2;

    if (delta_pos.length() > 0 && reset_mouse)
    {
        input.set_mouse_position(center_pos);
    }

    // If we don't have mouse capture, control rotation based on mouse movement.
    if (!mouse_captured)
    {
        if (m_viewport_control_frames > 2)
        {
            m_uncaptured_mouse_input_delay++;
        }
        else
        {
            m_uncaptured_mouse_input_delay = 0;

            forward_movement = 0.0f;
            right_movement = 0.0f;
            up_movement = 0.0f;
        }

        if (m_uncaptured_mouse_input_delay > 2 && mouse_over_viewport)
        {
            // Unreal style movement.

            // X=Left/Right Y=Up/Down
            if (lmb_down && rmb_down)
            {
                pan_right_movement += delta_pos.x * time.delta_seconds;
                pan_up_movement += -delta_pos.y * time.delta_seconds;
                delta_pos = vector2::zero;
            }
            // X-Axis=Turn Y-Axis=Forward/Backward
            else if (lmb_down)
            {
                if (fabs(delta_pos.y) > 3.0f)
                {            
                    y_plane_movement += -delta_pos.y * time.delta_seconds;
                }
                delta_pos.y = 0;
            }
            // Freelook
            else if (rmb_down)
            {
                // Nothing to do, just continue with delta_pos as-is.
            }
            else
            {
                delta_pos = vector2::zero;
            }
        }
        else
        {
            delta_pos = vector2::zero;
        }
    }

    component_filter<fly_camera_movement_component, const transform_component, const camera_component> filter(m_manager);
    for (size_t i = 0; i < filter.size(); i++)
    {
        object obj = filter.get_object(i);

        const transform_component* transform = filter.get_component<transform_component>(i);
        const camera_component* camera = filter.get_component<camera_component>(i);
        fly_camera_movement_component* movement = filter.get_component<fly_camera_movement_component>(i);
        
        vector3 target_position = transform->local_location;
        quat target_rotation = quat::identity;

        // If we have captured the mouse long enough for it to be stably locked to the screen center
        // then start applying the rotational movement.
        if (!mouse_captured)
        {
            movement->mouse_capture_frames = 0;
        }
        else
        {
            movement->mouse_capture_frames++;
        }

        if (movement->mouse_capture_frames > 2 || !mouse_captured)
        {
            movement->rotation_euler.y += (-delta_pos.x * movement->sensitivity);
            movement->rotation_euler.x = std::min(
                std::max(
                    movement->rotation_euler.x - (delta_pos.y * movement->sensitivity), 
                    math::pi * -(movement->max_vertical_angle * 0.5f)
                ), 
                math::pi * (movement->max_vertical_angle * 0.5f)
            );

            quat x_rotation = quat::angle_axis(movement->rotation_euler.y, vector3::up);
            quat y_rotation = quat::angle_axis(movement->rotation_euler.x, vector3::right);
            target_rotation = y_rotation * x_rotation;
        }

        // Apply movement input.
        target_position += (vector3::forward * target_rotation) * forward_movement * movement->speed;
        target_position += (vector3::right * target_rotation) * right_movement * movement->speed;
        target_position += (vector3::up * target_rotation) * up_movement * movement->speed;

        // Apply mouse delta movement.
        target_position += (vector3::forward * target_rotation) * mouse_delta_movement * movement->zoom_speed;

        // Apply uncaptured movement.
        vector3 y_plane_vector = y_plane.project(vector3::forward * target_rotation);
        target_position += y_plane_vector * y_plane_movement * movement->pan_speed;

        target_position += (vector3::right * target_rotation) * pan_right_movement * movement->pan_speed;
        target_position += vector3::up * pan_up_movement * movement->pan_speed;

        // Tell the transform system to move our camera to the new target transform.
        m_manager.get_system<transform_system>()->set_local_transform(obj, target_position, target_rotation, transform->local_scale);
    }
}

}; // namespace ws
