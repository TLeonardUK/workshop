// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.input_interface/input_interface.h"

#include "thirdparty/sdl2/include/SDL.h"

#include <array>

namespace ws {

class window;
class sdl_window;

// ================================================================================================
//  Implementation of input using the SDL library.
// ================================================================================================
class sdl_input_interface : public input_interface
{
public:
    sdl_input_interface(window* in_window);

    virtual void register_init(init_list& list) override;
    virtual void pump_events() override;

    virtual bool is_key_down(input_key key) override;
    virtual bool was_key_pressed(input_key key) override;
    virtual bool was_key_released(input_key key) override;

    virtual bool is_modifier_down(input_modifier key) override;

    virtual std::string get_clipboard_text() override;
    virtual void set_clipboard_text(const char* text) override;

    virtual vector2 get_mouse_position() override;
    virtual void set_mouse_position(const vector2& pos) override;

    virtual float get_mouse_wheel_delta(bool horizontal) override;

    virtual void set_mouse_cursor(input_cursor cursor) override;

    virtual void set_mouse_capture(bool capture) override;

    virtual void set_mouse_hidden(bool hidden) override;
    
protected:

    result<void> create_sdl(init_list& list);
    result<void> destroy_sdl();

    void update_key_state(int keyIndex, bool down);
    bool is_window_in_focus();

private:

    enum class key_state_flags : int
    {
        down = 1,
        pressed = 2,
        released = 4,
    };

    int m_mouse_x;
    int m_mouse_y;
    uint32_t m_mouse_state;
    const Uint8* m_keyboard_state;

    int m_mouse_wheel_vertical;
    int m_mouse_wheel_horizontal;

    std::array<int, (int)input_key::count> m_key_states;

    std::array<SDL_Cursor*, (int)input_cursor::count> m_mouse_cursors;

    sdl_window* m_window = nullptr;


};

}; // namespace ws
