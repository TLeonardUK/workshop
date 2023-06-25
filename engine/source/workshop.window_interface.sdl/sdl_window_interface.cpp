// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.window_interface.sdl/sdl_window_interface.h"
#include "workshop.window_interface.sdl/sdl_window.h"
#include "workshop.core/perf/profile.h"

#include "thirdparty/sdl2/include/SDL.h"

namespace ws {

sdl_window_interface::sdl_window_interface(platform_interface* platform_interface)
{
    m_platform_interface = dynamic_cast<sdl_platform_interface*>(platform_interface);
    db_assert_message(m_platform_interface != nullptr, "Platform interface is no of sdl type, incompatible.");
}

void sdl_window_interface::register_init(init_list& list)
{
    list.add_step(
        "Initialize SDL Windowing",
        [this, &list]() -> result<void> { return create_sdl(list); },
        [this]() -> result<void> { return destroy_sdl(); }
    );
}

result<void> sdl_window_interface::create_sdl(init_list& list)
{
    return true;
}

result<void> sdl_window_interface::destroy_sdl()
{
    return true;
}

void sdl_window_interface::pump_events()
{
}

std::unique_ptr<window> sdl_window_interface::create_window(
    const char* name,
    size_t width,
    size_t height,
    window_mode mode,
    ri_interface_type compatibility)
{
    std::unique_ptr<sdl_window> window = std::make_unique<sdl_window>(this, m_platform_interface);
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
