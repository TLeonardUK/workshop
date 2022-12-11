// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.windowing/windowing.h"

#include <unordered_map>

namespace ws {

class sdl_window;

// ================================================================================================
//  Implementation of windowing using the SDL library.
// ================================================================================================
class sdl_windowing : public windowing
{
public:

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

    void register_window(sdl_window* window);
    void unregister_window(sdl_window* window);

private:

    std::vector<sdl_window*> m_windows;

};

}; // namespace ws
