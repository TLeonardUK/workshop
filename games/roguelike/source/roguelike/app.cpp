// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "roguelike/app.h"

#include "workshop.core/filesystem/file.h"
#include "workshop.core/utils/frame_time.h"

#include "workshop.engine/engine/engine.h"

#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_object.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.renderer/objects/render_static_mesh.h"
#include "workshop.renderer/assets/model/model.h"

std::shared_ptr<ws::app> make_app()
{
    return std::make_shared<ws::rl_game_app>();
}

namespace ws {

std::string rl_game_app::get_name()
{
    return "roguelike";
}

void rl_game_app::configure_engine(engine& engine)
{
    engine.set_render_interface_type(ri_interface_type::dx12);
    engine.set_window_interface_type(window_interface_type::sdl);
    engine.set_input_interface_type(input_interface_type::sdl);
    engine.set_platform_interface_type(platform_interface_type::sdl);
    engine.set_window_mode(get_name(), 1920, 1080, ws::window_mode::windowed);
}

ws::result<void> rl_game_app::start()
{    
    auto& cmd_queue = get_engine().get_renderer().get_command_queue();
    auto& ass_manager = get_engine().get_asset_manager();

    m_view_id = cmd_queue.create_view("Main View");
    cmd_queue.set_view_viewport(m_view_id, recti(0, 0, static_cast<int>(get_engine().get_main_window().get_width()), static_cast<int>(get_engine().get_main_window().get_height())));
    cmd_queue.set_view_projection(m_view_id, 45.0f, 1.77f, 10.0f, 10000.0f);
    cmd_queue.set_object_transform(m_view_id, vector3(0.0f, 0.0f, -100.0f), quat::identity, vector3::one);

    asset_ptr<model> test_model = ass_manager.request_asset<model>("data:models/test_scenes/sponza/sponza.yaml", 0);
    //asset_ptr<model> test_model = ass_manager.request_asset<model>("data:models/test_scenes/cube/cube.yaml", 0);

    for (size_t i = 0; i < 10; i++)
    {
        render_object_id object_id = cmd_queue.create_static_mesh("Sponza");
        cmd_queue.set_static_mesh_model(object_id, test_model);
        cmd_queue.set_object_transform(object_id, vector3::zero, quat::identity, vector3::one);
    }

    m_on_step_delegate = get_engine().on_step.add_shared([this](const frame_time& time) {
        step(time);
    });

    return true;
}

ws::result<void> rl_game_app::stop()
{
    return true;
}

void rl_game_app::step(const frame_time& time)
{
    auto& cmd_queue = get_engine().get_renderer().get_command_queue();

    window& main_window = get_engine().get_main_window();
    input_interface& input = get_engine().get_input_interface();
    
    vector2 center_pos(main_window.get_width() * 0.5f, main_window.get_height() * 0.5f);
    vector2 delta_pos = (input.get_mouse_position() - center_pos);
    input.set_mouse_position(center_pos);

    constexpr float k_sensitivity = 0.001f;
    constexpr float k_speed = 100.0f;

    if (time.frame_count > 2)
    {
        m_view_rotation_euler.y += (-delta_pos.x * k_sensitivity);
        m_view_rotation_euler.x = std::min(std::max(m_view_rotation_euler.x - (delta_pos.y * k_sensitivity), math::pi * -0.4f), math::pi * 0.4f);

        quat x_rotation = quat::angle_axis(m_view_rotation_euler.y, vector3::up);
        quat y_rotation = quat::angle_axis(m_view_rotation_euler.x, vector3::right);
        m_view_rotation = y_rotation * x_rotation;
    }

    if (input.is_key_down(input_key::w))
    {
        m_view_position += (vector3::forward * m_view_rotation) * k_speed * time.delta_seconds;
    }
    if (input.is_key_down(input_key::s))
    {
        m_view_position -= (vector3::forward * m_view_rotation) * k_speed * time.delta_seconds;
    }
    if (input.is_key_down(input_key::a))
    {
        m_view_position -= (vector3::right * m_view_rotation) * k_speed * time.delta_seconds;
    }
    if (input.is_key_down(input_key::d))
    {
        m_view_position += (vector3::right * m_view_rotation) * k_speed * time.delta_seconds;
    }
    if (input.is_key_down(input_key::q))
    {
        m_view_position += (vector3::up * m_view_rotation) * k_speed * time.delta_seconds;
    }
    if (input.is_key_down(input_key::e))
    {
        m_view_position -= (vector3::up * m_view_rotation) * k_speed * time.delta_seconds;
    }

    cmd_queue.set_object_transform(m_view_id,
        m_view_position,
        m_view_rotation,
        vector3::one
    );


    /*
    cmd_queue.set_object_transform(m_view_id, 
        vector3(sin(time.elapsed_seconds * 0.1f) * 1200.0f, 100.0f, 0.0f),
        quat::angle_axis(fmod(time.elapsed_seconds * 0.2f, math::pi2), vector3::up),
        vector3::one
    );
    */
}

}; // namespace hg