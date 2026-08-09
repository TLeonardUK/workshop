// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.input_interface/input_interface.h"

namespace ws {

class window;
class platform_interface;

// ================================================================================================
//  Stub implementation of input_interface, performs no actual work. Useful for headless
//  builds or platforms that do not have a real input implementation available.
// ================================================================================================
class stub_input_interface : public input_interface
{
public:
    stub_input_interface(platform_interface* platform_interface, window* in_window);

    virtual void register_init(init_list& list) override;
    virtual void pump_events() override;

    virtual bool is_key_down(input_key key) override;
    virtual bool was_key_pressed(input_key key) override;
    virtual bool was_key_released(input_key key) override;
    virtual bool was_key_hit(input_key key) override;

    virtual std::string get_clipboard_text() override;
    virtual void set_clipboard_text(const char* text) override;

    virtual vector2 get_mouse_position() override;
    virtual void set_mouse_position(const vector2& pos) override;

    virtual float get_mouse_wheel_delta(bool horizontal) override;

    virtual void set_mouse_cursor(input_cursor cursor) override;

    virtual void set_mouse_capture(bool capture) override;
    virtual bool get_mouse_capture() override;

    virtual void set_mouse_hidden(bool hidden) override;

    virtual std::string get_input() override;

private:

    std::string m_clipboard_text;
    vector2 m_mouse_position;
    bool m_mouse_captured = false;

};

}; // namespace ws
