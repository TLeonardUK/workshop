// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.window_interface/window_interface.h"

#include <unordered_map>

namespace ws {

class sdl_window;
class platform_interface;
class sdl_platform_interface;

// ================================================================================================
//  Implementation of windowing using the SDL library.
// ================================================================================================
class sdl_window_interface : public window_interface
{
public:
    sdl_window_interface(platform_interface* platform_interface);

    virtual void register_init(init_list& list) override;
    virtual void pump_events() override;
    virtual std::unique_ptr<window> create_window(
        const char* name,
        size_t width,
        size_t height,
        window_mode mode,
        ri_interface_type compatibility) override;

protected:

    friend class sdl_window;

    result<void> create_sdl(init_list& list);
    result<void> destroy_sdl();

private:
    sdl_platform_interface* m_platform_interface;

    std::vector<sdl_window*> m_windows;

};

}; // namespace ws
