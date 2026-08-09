// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.window_interface/window_interface.h"

namespace ws {

class stub_window;
class platform_interface;

// ================================================================================================
//  Stub implementation of window_interface, performs no actual work. Useful for headless
//  builds or platforms that do not have a real windowing implementation available.
// ================================================================================================
class stub_window_interface : public window_interface
{
public:
    stub_window_interface(platform_interface* platform_interface);

    virtual void register_init(init_list& list) override;
    virtual void pump_events() override;
    virtual std::unique_ptr<window> create_window(
        const char* name,
        size_t width,
        size_t height,
        window_mode mode,
        ri_interface_type compatibility) override;

protected:

    friend class stub_window;

    result<void> create_stub(init_list& list);
    result<void> destroy_stub();

private:
    platform_interface* m_platform_interface;

};

}; // namespace ws
