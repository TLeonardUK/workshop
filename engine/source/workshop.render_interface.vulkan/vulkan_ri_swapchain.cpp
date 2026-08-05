// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_swapchain.h"

namespace ws {

namespace {

ri_texture::create_params make_backbuffer_params(window& for_window)
{
    ri_texture::create_params params;
    params.width = for_window.get_width();
    params.height = for_window.get_height();
    params.is_render_target = true;
    return params;
}

}; // namespace

vulkan_ri_swapchain::vulkan_ri_swapchain(window& for_window, const char* debug_name)
    : m_backbuffer(make_backbuffer_params(for_window), debug_name)
{
}

ri_texture& vulkan_ri_swapchain::next_backbuffer()
{
    return m_backbuffer;
}

void vulkan_ri_swapchain::present()
{
}

void vulkan_ri_swapchain::drain()
{
}

}; // namespace ws
