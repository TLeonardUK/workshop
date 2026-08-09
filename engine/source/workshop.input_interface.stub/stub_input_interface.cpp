// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.input_interface.stub/stub_input_interface.h"

namespace ws {

stub_input_interface::stub_input_interface(platform_interface* platform_interface, window* in_window)
{
}

void stub_input_interface::register_init(init_list& list)
{
}

void stub_input_interface::pump_events()
{
}

bool stub_input_interface::is_key_down(input_key key)
{
    return false;
}

bool stub_input_interface::was_key_pressed(input_key key)
{
    return false;
}

bool stub_input_interface::was_key_released(input_key key)
{
    return false;
}

bool stub_input_interface::was_key_hit(input_key key)
{
    return false;
}

std::string stub_input_interface::get_clipboard_text()
{
    return m_clipboard_text;
}

void stub_input_interface::set_clipboard_text(const char* text)
{
    m_clipboard_text = text;
}

vector2 stub_input_interface::get_mouse_position()
{
    return m_mouse_position;
}

void stub_input_interface::set_mouse_position(const vector2& pos)
{
    m_mouse_position = pos;
}

float stub_input_interface::get_mouse_wheel_delta(bool horizontal)
{
    return 0.0f;
}

void stub_input_interface::set_mouse_cursor(input_cursor cursor)
{
}

void stub_input_interface::set_mouse_capture(bool capture)
{
    m_mouse_captured = capture;
}

bool stub_input_interface::get_mouse_capture()
{
    return m_mouse_captured;
}

void stub_input_interface::set_mouse_hidden(bool hidden)
{
}

std::string stub_input_interface::get_input()
{
    return "";
}

}; // namespace ws
