// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "roguelike/app.h"

#include "workshop.core/filesystem/file.h"
#include "workshop.core/utils/frame_time.h"
#include "workshop.core/math/sphere.h"

#include "workshop.engine/engine/engine.h"

#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_object.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.renderer/objects/render_static_mesh.h"
#include "workshop.renderer/assets/model/model.h"

#include "workshop.renderer/render_imgui_manager.h"

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
    cmd_queue.set_view_projection(m_view_id, 45.0f, 1.77f, 10.0f, 20000.0f);
    cmd_queue.set_object_transform(m_view_id, vector3(0.0f, 100.0f, -250.0f), quat::identity, vector3::one);
    
    render_object_id object_id;

#if 0
    m_view_position = vector3(150.0f, 270.0f, -100.0f);
    m_view_rotation = quat::angle_axis(0.0f, vector3::up);

    object_id = cmd_queue.create_static_mesh("Sponza");
    cmd_queue.set_static_mesh_model(object_id, ass_manager.request_asset<model>("data:models/test_scenes/sponza/sponza.yaml", 0));
    cmd_queue.set_object_transform(object_id, vector3::zero, quat::identity, vector3::one);
    
    object_id = cmd_queue.create_static_mesh("Sponza Curtains");
    cmd_queue.set_static_mesh_model(object_id, ass_manager.request_asset<model>("data:models/test_scenes/sponza_curtains/sponza_curtains.yaml", 0));
    cmd_queue.set_object_transform(object_id, vector3::zero, quat::identity, vector3::one);

    object_id = cmd_queue.create_static_mesh("Cerberus");
    cmd_queue.set_static_mesh_model(object_id, ass_manager.request_asset<model>("data:models/test_scenes/cerberus/cerberus.yaml", 0));
    cmd_queue.set_object_transform(object_id, vector3(-400.0f, 200.0f, 150.0f), quat::identity, vector3::one);

    /*
    object_id = cmd_queue.create_static_mesh("Sponza Ivy");
    cmd_queue.set_static_mesh_model(object_id, ass_manager.request_asset<model>("data:models/test_scenes/sponza_ivy/sponza_ivy.yaml", 0));
    cmd_queue.set_object_transform(object_id, vector3::zero, quat::identity, vector3::one);

    object_id = cmd_queue.create_static_mesh("Sponza Trees");
    cmd_queue.set_static_mesh_model(object_id, ass_manager.request_asset<model>("data:models/test_scenes/sponza_trees/sponza_trees.yaml", 0));
    cmd_queue.set_object_transform(object_id, vector3(0.0f, 0.0f, 0.0f), quat::identity, vector3::one);
    */

    object_id = cmd_queue.create_reflection_probe("Reflection Probe");
    cmd_queue.set_object_transform(object_id, vector3(0.0, 200.0f, 0.0f), quat::identity, vector3(4000.f, 4000.0f, 4000.0f));

    object_id = cmd_queue.create_light_probe_grid("Light Probe Grid");
    cmd_queue.set_light_probe_grid_density(object_id, 200.0f);
    cmd_queue.set_object_transform(object_id, vector3(200.0, 1050.0f, -100.0f), quat::identity, vector3(3900.f, 2200.0f, 2200.0f));
    //cmd_queue.set_object_transform(object_id, vector3(200.0, 320.0f, -100.0f), quat::identity, vector3(2900.f, 500.0f, 1200.0f));
    //cmd_queue.set_object_transform(object_id, vector3(200.0, 200.0f, -100.0f), quat::identity, vector3(1450.f, 700.0f, 800.0f));
#else
    m_view_position = vector3(-900.0f, 400.0f, 0.0f);
    m_view_rotation = quat::angle_axis(0.0f, vector3::up);

    object_id = cmd_queue.create_static_mesh("Bistro Exterior");
    cmd_queue.set_static_mesh_model(object_id, ass_manager.request_asset<model>("data:models/test_scenes/bistro/bistro_exterior.yaml", 0));
    cmd_queue.set_object_transform(object_id, vector3::zero, quat::identity, vector3::one);

    object_id = cmd_queue.create_static_mesh("Bistro Interior");
    cmd_queue.set_static_mesh_model(object_id, ass_manager.request_asset<model>("data:models/test_scenes/bistro/bistro_interior.yaml", 0));
    cmd_queue.set_object_transform(object_id, vector3::zero, quat::identity, vector3::one);

    //object_id = cmd_queue.create_static_mesh("Bistro Wine");
    //cmd_queue.set_static_mesh_model(object_id, ass_manager.request_asset<model>("data:models/test_scenes/bistro/bistro_wine.yaml", 0));
    //cmd_queue.set_object_transform(object_id, vector3::zero, quat::identity, vector3::one);

    object_id = cmd_queue.create_reflection_probe("Reflection Probe");
    cmd_queue.set_object_transform(object_id, vector3(-900.0f, 400.0f, 0.0f), quat::identity, vector3(10000.f, 10000.0f, 10000.0f));

    object_id = cmd_queue.create_light_probe_grid("Light Probe Grid");
    cmd_queue.set_light_probe_grid_density(object_id, 200.0f);
    cmd_queue.set_object_transform(object_id, vector3(3000.0f, 2000.0f, 0.0f), quat::identity, vector3(20000.f, 4000.0f, 20000.0f));
#endif
    /*
    for (int x = 0; x < 150; x++)
    {
        object_id = cmd_queue.create_static_mesh("Cube");
        cmd_queue.set_static_mesh_model(object_id, ass_manager.request_asset<model>("data:models/test_scenes/cube/cube.yaml", 0));
        cmd_queue.set_object_transform(object_id, vector3(0.0f, 0.0f, 0.0f), quat::identity, vector3(50.0f, 50.0f, 50.0f));

        m_rotating_objects.push_back(object_id);
    }
    */

//    object_id = cmd_queue.create_reflection_probe_grid("Reflection Probe Grid");
//    cmd_queue.set_reflection_probe_grid_density(object_id, true);

#if 1
    object_id = cmd_queue.create_directional_light("Sun");
    cmd_queue.set_light_shadow_casting(object_id, true);
    cmd_queue.set_light_shadow_map_size(object_id, 2048);
    cmd_queue.set_light_shadow_max_distance(object_id, 10000.0f);
    cmd_queue.set_light_intensity(object_id, 1.0f);
    cmd_queue.set_object_transform(object_id, vector3(0.0f, 300.0f, 0.0f), quat::angle_axis(-math::halfpi * 0.85f, vector3::right) * quat::angle_axis(0.5f, vector3::forward), vector3::one);
#endif

    m_light_id = object_id;

#if 1
    object_id = cmd_queue.create_point_light("Point");
    cmd_queue.set_light_shadow_casting(object_id, false);
    cmd_queue.set_light_intensity(object_id, 1.0f);
    cmd_queue.set_light_range(object_id, 200.0f);
    cmd_queue.set_object_transform(object_id, vector3(500.0f, 150.0f, 500.0f), quat::identity, vector3::one);
#endif

#if 0
    for (size_t i = 0; i < 100; i++)
    {
        moving_light& light = m_moving_lights.emplace_back();
        light.distance = (rand() / static_cast<float>(RAND_MAX)) * 800.0f;
        light.angle = (rand() / static_cast<float>(RAND_MAX)) * math::pi2;
        light.speed = 10.0f + ((1.0f - ((rand() / static_cast<float>(RAND_MAX)) * 2.0f)) * 10.0f);
        light.range = 100.0f + ((rand() / static_cast<float>(RAND_MAX)) * 200.0f);
        light.height = 50.0f + ((rand() / static_cast<float>(RAND_MAX)) * 1000.0f);

        color color(
            rand() / static_cast<float>(RAND_MAX),
            rand() / static_cast<float>(RAND_MAX),
            rand() / static_cast<float>(RAND_MAX),
            1.0f
        );
        
        vector3 location = vector3(
            sin((0.0f + light.angle) * light.speed) * light.distance,
            light.height,
            cos((0.0f + light.angle) * light.speed) * light.distance
        );

        light.id = cmd_queue.create_point_light("Point");
        cmd_queue.set_light_intensity(light.id, 50.0f);
        cmd_queue.set_light_range(light.id, light.range);
        cmd_queue.set_light_color(light.id, color);
        cmd_queue.set_object_transform(light.id, location, quat::identity, vector3::one);
    }
#endif

#if 0
    object_id = cmd_queue.create_point_light("Point");
    cmd_queue.set_light_intensity(object_id, 50.0f);
    cmd_queue.set_light_range(object_id, 300.0f);
    cmd_queue.set_light_color(object_id, color::red);
    cmd_queue.set_object_transform(object_id, vector3(0.0f, 100.0f, 0.0f), quat::identity, vector3::one);
#endif

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
    render_command_queue& cmd_queue = get_engine().get_renderer().get_command_queue();

    window& main_window = get_engine().get_main_window();
    input_interface& input = get_engine().get_input_interface();
    
    static float angle = 0.0f;
    angle += 0.2f * time.delta_seconds;

    for (size_t i = 0; i < m_rotating_objects.size(); i++)
    {
        render_object_id id = m_rotating_objects[i];

        matrix4 transform = matrix4::translate(vector3(0.0f, 100.0f + ((i/10) * 120.0f), (i%10) * 120.0f)) * matrix4::rotation(quat::angle_axis(angle, vector3::up));

        vector3 location = vector3::zero * transform;

        cmd_queue.set_object_transform(id, location, quat::identity, vector3(50.0f, 50.0f, 50.0f));
    }

    /*
    for (int z = -2; z <= 2; z++)
    {
        for (int y = 0; y <= 4; y++)
        {
            for (int x = -3; x <= 3; x++)
            {
                vector3 location = (vector3(x, y, z) * 250.0f) + vector3(0.0f, 30.0f, 0.0f);
                cmd_queue.draw_sphere(sphere(location, 5.0f), color::white);
            }
        }
    }
    */

    #if 0
    //if (time.frame_count < 10)
    {
        float light_angle = angle;//0.0f;
        for (size_t i = 0; i < m_moving_lights.size(); i++)
        {
            moving_light& light = m_moving_lights[i];

            vector3 location = vector3(
                sin((light_angle + light.angle) * light.speed) * light.distance,
                light.height,
                cos((light_angle + light.angle) * light.speed) * light.distance
            );

            //cmd_queue.draw_sphere(sphere(location, 5.0f), color::white);
            cmd_queue.set_object_transform(light.id, location, quat::identity, vector3::one);
        }
    }
    #endif

    vector3 light_pos = vector3(0.0f, 50.0f, 0.0f);// vector3(-cos(angle * 3.0f) * 400.0f, 150.0f + (cos(angle*3.0f) * 200.0f), 0.0f);
    //vector3 light_pos = vector3(0.0f, 50.0f, 0.0f);
    quat light_rot =  quat::identity;//quat::angle_axis(angle * 0.5f, vector3::up);
    //quat light_rot = quat::angle_axis(angle * 3.0f, vector3::up) * quat::angle_axis(angle * 0.5f, vector3::right);
    //quat light_rot = quat::angle_axis(-math::halfpi*0.85f, vector3::right);
    //cmd_queue.set_object_transform(m_light_id, light_pos, light_rot, vector3::one);
//    cmd_queue.draw_sphere(sphere(light_pos, 100.0f), color::red);
//    cmd_queue.draw_arrow(light_pos, light_pos + (vector3::forward * light_rot) * 100.0f, color::green);

    if (input.get_mouse_capture())
    {
        vector2 center_pos(main_window.get_width() * 0.5f, main_window.get_height() * 0.5f);
        vector2 delta_pos = (input.get_mouse_position() - center_pos);
        if (delta_pos.length() > 0)
        {
            input.set_mouse_position(center_pos);
        }

        constexpr float k_sensitivity = 0.001f;
        constexpr float k_speed = 1500.0f;

        m_mouse_control_frames++;
        if (m_mouse_control_frames > 2)
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

        #if 1
        cmd_queue.set_object_transform(m_view_id,
            m_view_position,
            m_view_rotation,
            vector3::one
        );
        #endif

//        db_log(core, "position:%.2f,%.2f,%.2f", m_view_position.x, m_view_position.y, m_view_position.z);
    }
    else
    {
        m_mouse_control_frames = 0;
    }
}

}; // namespace hg