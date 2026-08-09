// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.window_interface/window.h"

namespace ws {

class stub_window_interface;

// ================================================================================================
//  Stub implementation of a window, performs no actual work.
// ================================================================================================
class stub_window : public window
{
public:

    stub_window(stub_window_interface* owner);
    virtual ~stub_window();

    virtual result<void> apply_changes() override;

    virtual void* get_platform_handle() override;

private:

    stub_window_interface* m_owner = nullptr;

};

}; // namespace ws
