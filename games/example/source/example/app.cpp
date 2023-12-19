// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "example/app.h"

#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"

#include "workshop.game_framework/systems/default_systems.h"

std::shared_ptr<ws::app> make_app()
{
    return std::make_shared<ws::example_game_app>();
}

namespace ws {

std::string example_game_app::get_name()
{
    return "example";
}

void example_game_app::configure_engine(engine& engine)
{
    engine.set_render_interface_type(ri_interface_type::dx12);
    engine.set_window_interface_type(window_interface_type::sdl);
    engine.set_input_interface_type(input_interface_type::sdl);
    engine.set_platform_interface_type(platform_interface_type::sdl);
    engine.set_physics_interface_type(physics_interface_type::jolt);
    engine.set_window_mode(get_name(), 1920, 1080, ws::window_mode::windowed);
    engine.set_system_registration_callback([](object_manager& obj_manager) {
        register_default_systems(obj_manager);
    });
}

ws::result<void> example_game_app::start()
{
    //get_engine().load_world("data:scenes/textured_cube.yaml");
    get_engine().load_world("data:scenes/sponza.yaml");
    //get_engine().load_world("data:scenes/ddgi_house.yaml");
    //get_engine().load_world("data:scenes/bistro.yaml");
    //get_engine().load_world("data:scenes/two_rooms.yaml");
    return true;
}

ws::result<void> example_game_app::stop()
{
    return true;
}

void example_game_app::step(const frame_time& time)
{
    // Nothing much to do here right at the moment.    
}

}; // namespace ws