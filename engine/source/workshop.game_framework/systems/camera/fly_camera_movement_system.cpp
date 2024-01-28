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

namespace ws {

fly_camera_movement_system::fly_camera_movement_system(object_manager& manager)
    : system(manager, "fly camera movement system")
{
    set_flags(system_flags::none);

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
    float mouse_delta = input.get_mouse_wheel_delta(false);
    vector2 mouse_position = input.get_mouse_position();


    // Calculate movement.
    float forward_movement = (static_cast<float>(w_down) - static_cast<float>(s_down)) * time.delta_seconds;
    float right_movement = (static_cast<float>(d_down) - static_cast<float>(a_down)) * time.delta_seconds;
    float up_movement = (static_cast<float>(e_down) - static_cast<float>(q_down)) * time.delta_seconds;

    // Calculate how much the mouse has moved from the center of the screen and reset
    // it to the center.
    vector2 center_pos = screen_size * 0.5f;
    vector2 delta_pos = (mouse_position - center_pos);

    if (delta_pos.length() > 0)
    {
        input.set_mouse_position(center_pos);
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

        // Apply rotation.
        movement->rotation_euler.y += (-delta_pos.x * movement->sensitivity);
        movement->rotation_euler.x = std::min(
            std::max(
                movement->rotation_euler.x - (delta_pos.y * movement->sensitivity), 
                math::pi * -(movement->max_vertical_angle * 0.5f)
            ), 
            math::pi * (movement->max_vertical_angle * 0.5f)
        );

        movement->rotation_euler.x = std::fmod(movement->rotation_euler.x, math::pi2);
        movement->rotation_euler.y = std::fmod(movement->rotation_euler.y, math::pi2);

        // Apply current rotation.
        quat x_rotation = quat::angle_axis(movement->rotation_euler.y, vector3::up);
        quat y_rotation = quat::angle_axis(movement->rotation_euler.x, vector3::right);
        target_rotation = y_rotation * x_rotation;

        // Apply movement input.
        target_position += (vector3::forward * target_rotation) * forward_movement * movement->speed;
        target_position += (vector3::right * target_rotation) * right_movement * movement->speed;
        target_position += (vector3::up * target_rotation) * up_movement * movement->speed;

        // Tell the transform system to move our camera to the new target transform.
        m_manager.get_system<transform_system>()->set_local_transform(obj, target_position, target_rotation, transform->local_scale);
    }
}

}; // namespace ws
