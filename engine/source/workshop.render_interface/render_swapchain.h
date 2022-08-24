// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

namespace ws {

class render_target;

// ================================================================================================
//  Swapchain for rendering to a window.
// ================================================================================================
class render_swapchain
{
public:

    // Gets the render target for the next back buffer to be rendered to.
    // This may block until a back buffer is available.
    virtual render_target& next_backbuffer() = 0;

    // Swaps the current back buffer and presents it for display. 
    virtual void present() = 0;

    // Blocks until the gpu has finished working on this swapchain.
    virtual void drain() = 0;

};

}; // namespace ws
