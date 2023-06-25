// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.window_interface.sdl/sdl_window.h"
#include "workshop.window_interface.sdl/sdl_window_interface.h"

#include "thirdparty/sdl2/include/SDL_syswm.h"

namespace ws {

sdl_window::sdl_window(sdl_window_interface* owner, sdl_platform_interface* platform)
    : m_owner(owner)
{
    m_event_delegate = platform->on_sdl_event.add_shared(std::bind(&sdl_window::handle_event, this, std::placeholders::_1));
}

sdl_window::~sdl_window()
{
    m_event_delegate = nullptr;
}

void* sdl_window::get_platform_handle()
{
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);

    SDL_GetWindowWMInfo(m_window, &info);

    return reinterpret_cast<void*>(info.info.win.window);
}

SDL_Window* sdl_window::get_sdl_handle()
{
    return m_window;
}

void sdl_window::handle_event(const SDL_Event* event)
{
    if (event->type == SDL_KEYDOWN)
    {
        // Switch between fullscreen and windowed.
        if (event->key.keysym.sym == SDLK_RETURN && 
            (event->key.keysym.mod & KMOD_ALT) != 0)
        {             
            switch (m_mode)
            {
            case window_mode::borderless:
            case window_mode::fullscreen:
                {
                    db_log(window, "User has toggled to windowed mode.");

                    m_last_fullscreen_mode = m_mode;
                    set_mode(window_mode::windowed);
                    break;
                }
            case window_mode::windowed:
                {
                    db_log(window, "User has toggled to fullscreen mode.");

                    set_mode(m_last_fullscreen_mode);
                    break;
                }
            }

            apply_changes();
        }
    }
}

result<void> sdl_window::apply_changes()
{
    if (m_window == nullptr)
    {
        int flags = SDL_WINDOW_ALLOW_HIGHDPI;

        if (m_compatibility != ri_interface_type::dx12)
        {
            db_error(window, "Requested compatibility of SDL window with unsupported renderer.");
            return standard_errors::invalid_parameter;
        }

        switch (m_mode)
        {
        case window_mode::windowed:
            {
                break;
            }
        case window_mode::fullscreen:
            {
                flags |= SDL_WINDOW_BORDERLESS;
                flags |= SDL_WINDOW_FULLSCREEN;
                break;
            }
        case window_mode::borderless:
            {
                flags |= SDL_WINDOW_BORDERLESS;
                flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
                break;
            }
        }

        m_window = SDL_CreateWindow(
            m_title.c_str(), 
            SDL_WINDOWPOS_CENTERED, 
            SDL_WINDOWPOS_CENTERED, 
            static_cast<int>(m_width), 
            static_cast<int>(m_height), 
            static_cast<SDL_WindowFlags>(flags));

        if (m_window == nullptr)
        {
            db_error(window, "SDL_CreateWindow failed with error: %s", SDL_GetError());
            return standard_errors::failed;
        }
    }
    else
    {
        if (m_title_dirty)
        {
            SDL_SetWindowTitle(m_window, m_title.c_str());
            m_title_dirty = false;
        }

        if (m_size_dirty || m_mode_dirty)
        {
            SDL_SetWindowSize(m_window, static_cast<int>(m_width), static_cast<int>(m_height));

            switch (m_mode)
            {
            case window_mode::windowed:
                {
                    SDL_SetWindowFullscreen(m_window, 0);
                    SDL_SetWindowBordered(m_window, SDL_TRUE);
                    break;
                }
            case window_mode::fullscreen:
                {
                    SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN);
                    SDL_SetWindowBordered(m_window, SDL_FALSE);
                    break;
                }
            case window_mode::borderless:
                {
                    SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                    SDL_SetWindowBordered(m_window, SDL_FALSE);
                    break;
                }
            }

            m_size_dirty = false;
            m_mode_dirty = false;
        }
    }    

    return true;
}

}; // namespace ws
