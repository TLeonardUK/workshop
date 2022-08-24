// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.windowing/window.h"

#include "thirdparty/sdl2/include/SDL.h"

namespace ws {

class sdl_windowing;

// ================================================================================================
//  Implementation of an window using SDL.
// ================================================================================================
class sdl_window : public window
{
public:

    sdl_window(sdl_windowing* owner);
    virtual ~sdl_window();

    virtual result<void> apply_changes() override;

    virtual void* get_platform_handle() override;

protected:
    friend class sdl_windowing;

    void handle_event(const SDL_Event& event);

private:

    SDL_Window* m_window = nullptr;
    sdl_windowing* m_owner = nullptr;

    window_mode m_last_fullscreen_mode = window_mode::borderless;

};

}; // namespace ws
