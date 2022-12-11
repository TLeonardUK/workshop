// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "roguelike/app.h"

#include "workshop.core/filesystem/file.h"

#include "workshop.engine/engine/engine.h"

std::shared_ptr<ws::app> make_app()
{
    return std::make_shared<rl::rl_game_app>();
}

namespace rl {

std::string rl_game_app::get_name()
{
    return "roguelike";
}

void rl_game_app::configure_engine(ws::engine& engine)
{
    engine.set_render_interface_type(ws::ri_interface_type::dx12);
    engine.set_windowing_type(ws::windowing_type::sdl);
    engine.set_window_mode(get_name(), 1280, 720, ws::window_mode::windowed);
}

ws::result<void> rl_game_app::start()
{    
    return true;
}

ws::result<void> rl_game_app::stop()
{
    return true;
}

}; // namespace hg