// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "example/app.h"

#include "workshop.engine/engine/engine.h"

#include "workshop.input_interface/input_interface.h"
#include "workshop.physics_interface/physics_interface.h"
#include "workshop.platform_interface/platform_interface.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.window_interface/window_interface.h"

#include "workshop.core/app/app.h"
#include "workshop.core/utils/frame_time.h"
#include "workshop.core/utils/result.h"
#include "workshop.engine/ecs/object_manager.h"
#include "workshop.game_framework/systems/default_systems.h"
#include "workshop.window_interface/window.h"

#include <memory>
#include <string>

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