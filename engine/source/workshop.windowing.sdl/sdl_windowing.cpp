// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.windowing.sdl/sdl_windowing.h"
#include "workshop.windowing.sdl/sdl_window.h"
#include "workshop.engine/app/engine_app.h"
#include "workshop.core/perf/profile.h"

#include "thirdparty/sdl2/include/SDL.h"

namespace ws {

void sdl_windowing::register_init(init_list& list)
{
    list.add_step(
        "Initialize SDL",
        [this, &list]() -> result<void> { return create_sdl(list); },
        [this]() -> result<void> { return destroy_sdl(); }
    );
}

result<void> sdl_windowing::create_sdl(init_list& list)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        db_error(window, "SDL_Init failed with error: %s", SDL_GetError());
        return false;
    }
    return true;
}

result<void> sdl_windowing::destroy_sdl()
{
    SDL_Quit();
    return true;
}

void sdl_windowing::register_window(sdl_window* window)
{
    m_windows.push_back(window);
}

void sdl_windowing::unregister_window(sdl_window* window)
{
    if (auto iter = std::find(m_windows.begin(), m_windows.end(), window); iter != m_windows.end())
    {
        m_windows.erase(iter);
    }
}

void sdl_windowing::pump_events()
{
    profile_marker(profile_colors::system, "pump window events");

    engine_app& eapp = static_cast<engine_app&>(engine_app::instance());

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        for (auto& window : m_windows)
        {
            window->handle_event(event);
        }

        if (event.type == SDL_QUIT)
        {
            db_log(window, "User requested application close.");

            eapp.quit();
        }
    }
}

std::unique_ptr<window> sdl_windowing::create_window(
    const char* name,
    size_t width,
    size_t height,
    window_mode mode,
    render_interface_type compatibility)
{
    std::unique_ptr<sdl_window> window = std::make_unique<sdl_window>(this);
    window->set_title(name);
    window->set_width(width);
    window->set_height(height);
    window->set_mode(mode);
    window->set_compatibility(compatibility);
    if (!window->apply_changes())
    {
        return nullptr;
    }

    return std::move(window);
}

}; // namespace ws
