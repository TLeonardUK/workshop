// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/render_interface.h"

#include <string>

namespace ws {

// ================================================================================================
//  What style this window should show display itself in.
// ================================================================================================
enum class window_mode
{
    // Window in middle of screen with standard border style.
    windowed,

    // Exclusive fullscreen.
    fullscreen,

    // Window without border that fills entire screen.
    borderless,
};

// ================================================================================================
//  Interface for an individual platform window.
// ================================================================================================
class window
{
public:

    // Gets or sets the various metrics that define the style of the window.

    void set_title(const std::string& title);
    std::string get_title();

    void set_width(size_t value);
    size_t get_width();

    void set_height(size_t value);
    size_t get_height();

    void set_mode(window_mode value);
    window_mode get_mode();

    render_interface_type get_compatibility();

    // Applies any changes to the windows metrics, this will block until
    // the settings have been fully applied.
    virtual result<void> apply_changes() = 0;

    // Gets a platform specific OS handle for this window. 
    virtual void* get_platform_handle() = 0;

protected:

    friend class sdl_windowing;

    void set_compatibility(render_interface_type value);

protected:

    bool m_title_dirty = false;
    bool m_size_dirty = false;
    bool m_mode_dirty = false;

    std::string m_title;
    size_t m_width;
    size_t m_height;
    window_mode m_mode;
    render_interface_type m_compatibility;

};

}; // namespace ws
