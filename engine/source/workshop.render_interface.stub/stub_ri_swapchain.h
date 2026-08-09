// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_swapchain.h"
#include "workshop.render_interface.stub/stub_ri_texture.h"
#include "workshop.window_interface/window.h"

namespace ws {

// ================================================================================================
//  Stub implementation of a swapchain, performs no actual work.
// ================================================================================================
class stub_ri_swapchain : public ri_swapchain
{
public:
    stub_ri_swapchain(window& for_window, const char* debug_name);

    virtual ri_texture& next_backbuffer() override;
    virtual void present() override;
    virtual void drain() override;

private:
    stub_ri_texture m_backbuffer;

};

}; // namespace ws
