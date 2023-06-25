// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.platform_interface.sdl/sdl_platform_interface.h"
#include "workshop.core/perf/profile.h"
#include "workshop.core/app/app.h"

namespace ws {

void sdl_platform_interface::register_init(init_list& list)
{
    list.add_step(
        "Initialize SDL Platform",
        [this, &list]() -> result<void> { return create_sdl(list); },
        [this]() -> result<void> { return destroy_sdl(); }
    );
}

result<void> sdl_platform_interface::create_sdl(init_list& list)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        db_error(window, "SDL_Init failed with error: %s", SDL_GetError());
        return false;
    }
    
    SDL_DisableScreenSaver();

    return true;
}

result<void> sdl_platform_interface::destroy_sdl()
{
    SDL_EnableScreenSaver();
    SDL_Quit();
    return true;
}

void sdl_platform_interface::pump_events()
{
    profile_marker(profile_colors::system, "pump sdl events");

    app& eapp = app::instance();

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            db_log(window, "User requested application close.");

            eapp.quit();
        }
        else
        {
            on_sdl_event.broadcast(&event);
        }
    }
}

}; // namespace ws
