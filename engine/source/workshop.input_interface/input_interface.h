// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"
#include "workshop.core/math/vector2.h"

namespace ws {

// ================================================================================================
//  Types of input interface implementations available. Make sure to update if you add new ones.
// ================================================================================================
enum class input_interface_type
{
    sdl
};

// ================================================================================================
//  All the different keyboard keys that can be queried.
// ================================================================================================
enum class input_key
{
    invalid,

    a,
    b,
    c,
    d,
    e,
    f,
    g,
    h,
    i,
    j,
    k,
    l,
    m,
    n,
    o,
    p,
    q,
    r,
    s,
    t,
    u,
    v,
    w,
    x,
    y,
    z,

    num1,
    num2,
    num3,
    num4,
    num5,
    num6,
    num7,
    num8,
    num9,
    num0,

    enter,
    escape,
    backspace,
    tab,
    space,
    minus,
    equals,
    left_bracket,
    right_bracket,
    backslash,

    semicolon,
    apostrophe,
    grave,
    comma,
    period,
    slash,

    caps_lock,

    f1,
    f2,
    f3,
    f4,
    f5,
    f6,
    f7,
    f8,
    f9,
    f10,
    f11,
    f12,
    f13,
    f14,
    f15,
    f16,
    f17,
    f18,
    f19,
    f20,
    f21,
    f22,
    f23,
    f24,

    print_screen,
    scroll_lock,
    pause,
    insert,
    home,
    page_up,
    del,
    end,
    page_down,

    right,
    left,
    down,
    up,

    keypad_divide,
    keypad_multiply,
    keypad_minus,
    keypad_plus,
    keypad_enter,
    keypad_1,
    keypad_2,
    keypad_3,
    keypad_4,
    keypad_5,
    keypad_6,
    keypad_7,
    keypad_8,
    keypad_9,
    keypad_0,
    keypad_period,

    left_ctrl,
    left_shift,
    left_alt,
    left_gui,
    right_ctrl,
    right_shift,
    right_alt,
    right_gui,

    mouse_0,
    mouse_left = mouse_0,
    mouse_1,
    mouse_middle = mouse_1,
    mouse_2,
    mouse_right = mouse_2,
    mouse_3,
    mouse_4,
    mouse_5,

    // Special modifier keys.
    shift,
    ctrl,
    alt,
    gui,

    count
};

static const char* input_key_strings[static_cast<int>(input_key::count)] = {
    "invalid",

    "a",
    "b",
    "c",
    "d",
    "e",
    "f",
    "g",
    "h",
    "i",
    "j",
    "k",
    "l",
    "m",
    "n",
    "o",
    "p",
    "q",
    "r",
    "s",
    "t",
    "u",
    "v",
    "w",
    "x",
    "y",
    "z",

    "numpad 1",
    "numpad 2",
    "numpad 3",
    "numpad 4",
    "numpad 5",
    "numpad 6",
    "numpad 7",
    "numpad 8",
    "numpad 9",
    "numpad 0",

    "enter",
    "escape",
    "backspace",
    "tab",
    "space",
    "-",
    "=",
    "[",
    "]",
    "\\",

    ";",
    "'",
    "grave",
    ",",
    ".",
    "slash",

    "capslock",

    "f1",
    "f2",
    "f3",
    "f4",
    "f5",
    "f6",
    "f7",
    "f8",
    "f9",
    "f10",
    "f11",
    "f12",
    "f13",
    "f14",
    "f15",
    "f16",
    "f17",
    "f18",
    "f19",
    "f20",
    "f21",
    "f22",
    "f23",
    "f24",

    "print screen",
    "scroll lock",
    "pause",
    "insert",
    "home",
    "page up",
    "delete",
    "end",
    "page down",

    "right",
    "left",
    "down",
    "up",

    "keypad /",
    "keypad *",
    "keypad -",
    "keypad +",
    "keypad enter",
    "keypad 1",
    "keypad 2",
    "keypad 3",
    "keypad 4",
    "keypad 5",
    "keypad 6",
    "keypad 7",
    "keypad 8",
    "keypad 9",
    "keypad 0",
    "keypad .",

    "left ctrl",
    "left shift",
    "left alt",
    "left gui",
    "right ctrl",
    "right shift",
    "right alt",
    "right gui",

    "mouse left",
    "mouse middle",
    "mouse right",
    "mouse 3",
    "mouse 4",
    "mouse 5",

    "shift",
    "ctrl",
    "alt",
    "gui"
};

// ================================================================================================
//  All the different cursor icons can be shown on the mouse cursor.
// ================================================================================================
enum class input_cursor
{
    none,

    arrow,
    ibeam,
    wait,
    crosshair,
    wait_arrow,
    size_nwse,
    size_nesw,
    size_we,
    size_ns,
    size_all,
    no,
    hand,

    count
};

// ================================================================================================
//  Engine interface for input implementation.
// ================================================================================================
class input_interface
{
public:

    // Registers all the steps required to initialize the input system.
    // Interacting with this class without successfully running these steps is undefined.
    virtual void register_init(init_list& list) = 0;

    // Processes and dispatches any events recieved.
    virtual void pump_events() = 0;

    // Checks the state of a given key.
    virtual bool is_key_down(input_key key) = 0;
    virtual bool was_key_pressed(input_key key) = 0;
    virtual bool was_key_released(input_key key) = 0;

    // Gets or sets the current clipboard text.
    virtual std::string get_clipboard_text() = 0;
    virtual void set_clipboard_text(const char* text) = 0;

    // Gets the current mouse position.
    virtual vector2 get_mouse_position() = 0;
    virtual void set_mouse_position(const vector2& pos) = 0;

    // Gets how far the mouse wheel has been rolled since the last frame.
    virtual float get_mouse_wheel_delta(bool horizontal) = 0;

    // Sets the current icon displayed on the mouse cursor.
    virtual void set_mouse_cursor(input_cursor cursor) = 0;

    // Sets if the mouse is constrained within the application window.
    virtual void set_mouse_capture(bool capture) = 0;
    virtual bool get_mouse_capture() = 0;

    // Sets if the mouse is globally hidden or visible.
    virtual void set_mouse_hidden(bool hidden) = 0;

    // Gets input that has been typed in over the last frame.
    virtual std::string get_input() = 0;

};

}; // namespace ws
