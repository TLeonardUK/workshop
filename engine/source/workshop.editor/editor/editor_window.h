// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

namespace ws {

// Defines the location the window should be docked to by default.
enum class editor_window_layout
{
    top,
    bottom_left,
    bottom_right,
    left_bottom,
    left_top,
    //right,
    
    center_top_left,
    center_top_right,
    center_bottom_left,
    center_bottom_right, 

    popup,
    overlay
};

// ================================================================================================
//  Base class for all editor window classes.
// ================================================================================================
class editor_window
{
public:

    // Draws the window with imgui.
    virtual void draw() = 0;

    // Gets an imgui id of the window.
    virtual const char* get_window_id() = 0;

    // Gets the default layout of the window.
    virtual editor_window_layout get_layout() = 0;

    // Forces a window to open.
    void open();

    // Forces a window to close.
    void close();

    // Returns true if the window is open.
    bool is_open();

protected:

    bool m_open = true;

};

}; // namespace ws
