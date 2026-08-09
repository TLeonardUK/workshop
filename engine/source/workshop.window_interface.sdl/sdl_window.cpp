// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.window_interface.sdl/sdl_window.h"
#include "workshop.window_interface.sdl/sdl_window_interface.h"

#include "thirdparty/sdl2/include/SDL_syswm.h"
#include <functional>
#include <thirdparty/sdl2/include/SDL_error.h>
#include <thirdparty/sdl2/include/SDL_events.h>
#include <thirdparty/sdl2/include/SDL_keycode.h>
#include <thirdparty/sdl2/include/SDL_stdinc.h>
#include <thirdparty/sdl2/include/SDL_version.h>
#include <thirdparty/sdl2/include/SDL_video.h>
#include <workshop.core/debug/debug.h>
#include <workshop.core/debug/log.h>
#include <workshop.core/utils/result.h>
#include <workshop.platform_interface.sdl/sdl_platform_interface.h>
#include <workshop.render_interface/ri_interface.h>
#include <workshop.window_interface/window.h>

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

#if defined(WS_WINDOWS)
    return reinterpret_cast<void*>(info.info.win.window);
#elif defined(WS_LINUX)
    return nullptr;
#endif
}

SDL_Window* sdl_window::get_sdl_handle()
{
    return m_window;
}

void sdl_window::handle_event(const SDL_Event* event)
{
    if (event->type == SDL_WINDOWEVENT)
    {
        int x, y, width, height;
        SDL_GetWindowPosition(m_window, &x, &y);
        SDL_GetWindowSize(m_window, &width, &height);

        db_move_console(x, y + height, width, 200);        

        m_width = width;
        m_height = height;
    }
    else if (event->type == SDL_KEYDOWN)
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

        bool compatible_renderer = false;
#if defined(WS_WINDOWS)
        compatible_renderer |= (m_compatibility == ri_interface_type::dx12);
#endif
#if defined(WS_WINDOWS) || defined(WS_LINUX)
        compatible_renderer |= (m_compatibility == ri_interface_type::vulkan);
#endif

        if (!compatible_renderer)
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

        if (m_compatibility == ri_interface_type::vulkan) {
            flags |= SDL_WINDOW_VULKAN;
        }

#ifndef WS_RELEASE
        flags |= SDL_WINDOW_RESIZABLE;
#endif

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
