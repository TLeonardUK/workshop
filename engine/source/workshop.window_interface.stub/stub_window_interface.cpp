// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.window_interface.stub/stub_window_interface.h"
#include "workshop.window_interface.stub/stub_window.h"

namespace ws {

stub_window_interface::stub_window_interface(platform_interface* platform_interface)
    : m_platform_interface(platform_interface)
{
}

void stub_window_interface::register_init(init_list& list)
{
    list.add_step(
        "Initialize Stub Windowing",
        [this, &list]() -> result<void> { return create_stub(list); },
        [this]() -> result<void> { return destroy_stub(); }
    );
}

result<void> stub_window_interface::create_stub(init_list& list)
{
    return true;
}

result<void> stub_window_interface::destroy_stub()
{
    return true;
}

void stub_window_interface::pump_events()
{
}

std::unique_ptr<window> stub_window_interface::create_window(
    const char* name,
    size_t width,
    size_t height,
    window_mode mode,
    ri_interface_type compatibility)
{
    std::unique_ptr<stub_window> window = std::make_unique<stub_window>(this);
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
