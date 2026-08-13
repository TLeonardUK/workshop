// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_swapchain.h"
#include "workshop.render_interface.vulkan/vulkan_ri_texture.h"
#include "workshop.window_interface/window.h"

namespace ws {

// ================================================================================================
//  Implementation of a swapchain using Vulkan.
// ================================================================================================
class vulkan_ri_swapchain : public ri_swapchain
{
public:
    vulkan_ri_swapchain(window& for_window, const char* debug_name);

    virtual ri_texture& next_backbuffer() override;
    virtual void present() override;
    virtual void drain() override;

private:
    vulkan_ri_texture m_backbuffer;

};

}; // namespace ws
