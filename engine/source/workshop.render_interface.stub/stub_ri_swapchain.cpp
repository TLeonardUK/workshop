// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.stub/stub_ri_swapchain.h"

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

stub_ri_swapchain::stub_ri_swapchain(window& for_window, const char* debug_name)
    : m_backbuffer(make_backbuffer_params(for_window), debug_name)
{
}

ri_texture& stub_ri_swapchain::next_backbuffer()
{
    return m_backbuffer;
}

void stub_ri_swapchain::present()
{
}

void stub_ri_swapchain::drain()
{
}

}; // namespace ws
