// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.input_interface/input_interface.h"
#include "workshop.platform_interface.sdl/sdl_platform_interface.h"

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
    sdl_input_interface(platform_interface* platform_interface, window* in_window);

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
    virtual bool get_mouse_capture() override;

    virtual void set_mouse_hidden(bool hidden) override;

    virtual std::string get_input() override;
    
protected:

    result<void> create_sdl(init_list& list);
    result<void> destroy_sdl();

    void update_key_state(int keyIndex, bool down);
    bool is_window_in_focus();

    void handle_event(const SDL_Event* event);

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

    std::array<int, (int)input_key::count> m_key_states;

    std::array<SDL_Cursor*, (int)input_cursor::count> m_mouse_cursors;

    sdl_window* m_window = nullptr;

    std::string m_current_input = "";
    std::string m_pending_input = "";

    float m_mouse_wheel_vertical = 0.0f;
    float m_mouse_wheel_horizontal = 0.0f;

    float m_pending_mouse_wheel_vertical = 0.0f;
    float m_pending_mouse_wheel_horizontal = 0.0f;

    bool m_mouse_captured = false;

    sdl_platform_interface* m_platform = nullptr;

    decltype(sdl_platform_interface::on_sdl_event)::delegate_ptr m_event_delegate;

};

}; // namespace ws
