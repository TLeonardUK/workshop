// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.platform_interface.sdl/sdl_platform_interface.h"
#include "workshop.window_interface/window.h"

#include "thirdparty/sdl2/include/SDL.h"

namespace ws {

class sdl_window_interface;

// ================================================================================================
//  Implementation of an window using SDL.
// ================================================================================================
class sdl_window : public window
{
public:

    sdl_window(sdl_window_interface* owner, sdl_platform_interface* platform);
    virtual ~sdl_window();

    virtual result<void> apply_changes() override;

    virtual void* get_platform_handle() override;

    SDL_Window* get_sdl_handle();

protected:
    friend class sdl_window_interface;

    void handle_event(const SDL_Event* event);

private:

    SDL_Window* m_window = nullptr;
    sdl_window_interface* m_owner = nullptr;

    window_mode m_last_fullscreen_mode = window_mode::borderless;

    decltype(sdl_platform_interface::on_sdl_event)::delegate_ptr m_event_delegate;

};

}; // namespace ws
