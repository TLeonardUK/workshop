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
    engine.set_windowing_type(windowing_type::sdl);
    engine.set_window_mode(get_name(), 1920, 1080, ws::window_mode::windowed);
}

ws::result<void> rl_game_app::start()
{    
    auto& cmd_queue = get_engine().get_renderer().get_command_queue();
    auto& ass_manager = get_engine().get_asset_manager();

    m_view_id = cmd_queue.create_view("Main View");
    cmd_queue.set_view_viewport(m_view_id, recti(0, 0, static_cast<int>(get_engine().get_main_window().get_width()), static_cast<int>(get_engine().get_main_window().get_height())));
    cmd_queue.set_view_projection(m_view_id, 45.0f, 1.77f, 0.1f, 10000.0f);
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

    cmd_queue.set_object_transform(m_view_id, 
        vector3(sin(time.elapsed_seconds * 0.1f) * 1200.0f, 100.0f, 0.0f),
        quat::angle_axis(fmod(time.elapsed_seconds * 0.2f , math::pi2), vector3::up), 
        vector3::one
    );
}

}; // namespace hg