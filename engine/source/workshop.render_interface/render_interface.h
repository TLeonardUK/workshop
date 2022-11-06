// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"

namespace ws {

class window;
class render_swapchain;
class render_fence;
class render_command_queue;
class render_command_list;
class render_shader_compiler;

// ================================================================================================
//  Types of renderer implementations available. Make sure to update if you add new ones.
// ================================================================================================
enum class render_interface_type
{
#ifdef WS_WINDOWS
    dx12
#endif
};

// ================================================================================================
//  Engine interface for all rendering functionality.
// ================================================================================================
class render_interface
{
public:

    // Registers all the steps required to initialize the rendering system.
    // Interacting with this class without successfully running these steps is undefined.
    virtual void register_init(init_list& list) = 0;

    // Informs the renderer that a new frame is starting to be rendered. The 
    // render can use this notification to update per-frame allocations and do
    // any general bookkeeping required.
    virtual void new_frame() = 0;

    // Creates a swapchain for rendering to the given window.
    virtual std::unique_ptr<render_swapchain> create_swapchain(window& for_window, const char* debug_name = nullptr) = 0;
    
    // Creates a fence for syncronisation between the cpu and gpu.
    virtual std::unique_ptr<render_fence> create_fence(const char* debug_name = nullptr) = 0;

    // Creates an object describing the state of the graphics pipeline durng a draw call.
    // virtual std::unique_ptr<pipeline> create_pipeline(const char* debug_name = nullptr) = 0;

    // Creates a class to handle compiling shaders for offline use.
    virtual std::unique_ptr<render_shader_compiler> create_shader_compiler() = 0;

    // Gets the main graphics command queue responsible for raster ops.
    virtual render_command_queue& get_graphics_queue() = 0;

    // Gets the maximum number of frames that can be in flight at the same time.
    virtual size_t get_pipeline_depth() = 0;


};

}; // namespace ws
