// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.input_interface.sdl/sdl_input_interface.h"
#include "workshop.window_interface.sdl/sdl_window_interface.h"
#include "workshop.window_interface.sdl/sdl_window.h"
#include "workshop.platform_interface/platform_interface.h"
#include "workshop.platform_interface.sdl/sdl_platform_interface.h"
#include "workshop.core/perf/profile.h"

#include "thirdparty/sdl2/include/SDL.h"

namespace
{
    int g_input_key_to_scanecode[(int)ws::input_key::count] = {

        0,

        SDL_SCANCODE_A,
        SDL_SCANCODE_B,
        SDL_SCANCODE_C,
        SDL_SCANCODE_D,
        SDL_SCANCODE_E,
        SDL_SCANCODE_F,
        SDL_SCANCODE_G,
        SDL_SCANCODE_H,
        SDL_SCANCODE_I,
        SDL_SCANCODE_J,
        SDL_SCANCODE_K,
        SDL_SCANCODE_L,
        SDL_SCANCODE_M,
        SDL_SCANCODE_N,
        SDL_SCANCODE_O,
        SDL_SCANCODE_P,
        SDL_SCANCODE_Q,
        SDL_SCANCODE_R,
        SDL_SCANCODE_S,
        SDL_SCANCODE_T,
        SDL_SCANCODE_U,
        SDL_SCANCODE_V,
        SDL_SCANCODE_W,
        SDL_SCANCODE_X,
        SDL_SCANCODE_Y,
        SDL_SCANCODE_Z,

        SDL_SCANCODE_1,
        SDL_SCANCODE_2,
        SDL_SCANCODE_3,
        SDL_SCANCODE_4,
        SDL_SCANCODE_5,
        SDL_SCANCODE_6,
        SDL_SCANCODE_7,
        SDL_SCANCODE_8,
        SDL_SCANCODE_9,
        SDL_SCANCODE_0,

        SDL_SCANCODE_RETURN,
        SDL_SCANCODE_ESCAPE,
        SDL_SCANCODE_BACKSPACE,
        SDL_SCANCODE_TAB,
        SDL_SCANCODE_SPACE,
        SDL_SCANCODE_MINUS,
        SDL_SCANCODE_EQUALS,
        SDL_SCANCODE_LEFTBRACKET,
        SDL_SCANCODE_RIGHTBRACKET,
        SDL_SCANCODE_BACKSLASH,
        SDL_SCANCODE_NONUSHASH,

        SDL_SCANCODE_SEMICOLON,
        SDL_SCANCODE_APOSTROPHE,
        SDL_SCANCODE_GRAVE,
        SDL_SCANCODE_COMMA,
        SDL_SCANCODE_PERIOD,
        SDL_SCANCODE_SLASH,

        SDL_SCANCODE_CAPSLOCK,

        SDL_SCANCODE_F1,
        SDL_SCANCODE_F2,
        SDL_SCANCODE_F3,
        SDL_SCANCODE_F4,
        SDL_SCANCODE_F5,
        SDL_SCANCODE_F6,
        SDL_SCANCODE_F7,
        SDL_SCANCODE_F8,
        SDL_SCANCODE_F9,
        SDL_SCANCODE_F10,
        SDL_SCANCODE_F11,
        SDL_SCANCODE_F12,
        SDL_SCANCODE_F13,
        SDL_SCANCODE_F14,
        SDL_SCANCODE_F15,
        SDL_SCANCODE_F16,
        SDL_SCANCODE_F17,
        SDL_SCANCODE_F18,
        SDL_SCANCODE_F19,
        SDL_SCANCODE_F20,
        SDL_SCANCODE_F21,
        SDL_SCANCODE_F22,
        SDL_SCANCODE_F23,
        SDL_SCANCODE_F24,

        SDL_SCANCODE_PRINTSCREEN,
        SDL_SCANCODE_SCROLLLOCK,
        SDL_SCANCODE_PAUSE,
        SDL_SCANCODE_INSERT,
        SDL_SCANCODE_HOME,
        SDL_SCANCODE_PAGEUP,
        SDL_SCANCODE_DELETE,
        SDL_SCANCODE_END,
        SDL_SCANCODE_PAGEDOWN,

        SDL_SCANCODE_RIGHT,
        SDL_SCANCODE_LEFT,
        SDL_SCANCODE_DOWN,
        SDL_SCANCODE_UP,

        SDL_SCANCODE_KP_DIVIDE,
        SDL_SCANCODE_KP_MULTIPLY,
        SDL_SCANCODE_KP_MINUS,
        SDL_SCANCODE_KP_PLUS,
        SDL_SCANCODE_KP_ENTER,
        SDL_SCANCODE_KP_1,
        SDL_SCANCODE_KP_2,
        SDL_SCANCODE_KP_3,
        SDL_SCANCODE_KP_4,
        SDL_SCANCODE_KP_5,
        SDL_SCANCODE_KP_6,
        SDL_SCANCODE_KP_7,
        SDL_SCANCODE_KP_8,
        SDL_SCANCODE_KP_9,
        SDL_SCANCODE_KP_0,
        SDL_SCANCODE_KP_PERIOD,

        SDL_SCANCODE_LCTRL,
        SDL_SCANCODE_LSHIFT,
        SDL_SCANCODE_LALT,
        SDL_SCANCODE_LGUI,
        SDL_SCANCODE_RCTRL,
        SDL_SCANCODE_RSHIFT,
        SDL_SCANCODE_RALT,
        SDL_SCANCODE_RGUI,

        0,
        0,
        0,
        0,
        0
    };
};

namespace ws {

sdl_input_interface::sdl_input_interface(platform_interface* platform_interface, window* window)
{
    m_platform = dynamic_cast<sdl_platform_interface*>(platform_interface);
    m_window = dynamic_cast<sdl_window*>(window);

    db_assert_message(m_platform != nullptr, "Platform provided to input interface is not sdl. Input interface is not compatible.");
    db_assert_message(m_window != nullptr, "Window provided to input interface is not an sdl window. Input interface is not compatible.");
}

void sdl_input_interface::register_init(init_list& list)
{
    list.add_step(
        "Initialize SDL Input",
        [this, &list]() -> result<void> { return create_sdl(list); },
        [this]() -> result<void> { return destroy_sdl(); }
    );
}

result<void> sdl_input_interface::create_sdl(init_list& list)
{
    m_event_delegate = m_platform->on_sdl_event.add_shared(std::bind(&sdl_input_interface::handle_event, this, std::placeholders::_1));

    m_mouse_cursors[(int)input_cursor::arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    m_mouse_cursors[(int)input_cursor::ibeam] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    m_mouse_cursors[(int)input_cursor::wait] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
    m_mouse_cursors[(int)input_cursor::crosshair] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
    m_mouse_cursors[(int)input_cursor::wait_arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAITARROW);
    m_mouse_cursors[(int)input_cursor::size_nwse] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
    m_mouse_cursors[(int)input_cursor::size_nesw] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
    m_mouse_cursors[(int)input_cursor::size_we] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
    m_mouse_cursors[(int)input_cursor::size_ns] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
    m_mouse_cursors[(int)input_cursor::size_all] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
    m_mouse_cursors[(int)input_cursor::no] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);
    m_mouse_cursors[(int)input_cursor::hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);

    return true;
}

result<void> sdl_input_interface::destroy_sdl()
{
    for (size_t i = 0; i < (int)input_cursor::count; i++)
    {
        SDL_Cursor*& cursor = m_mouse_cursors[i];
        if (cursor != nullptr)
        {
            SDL_FreeCursor(cursor);
            cursor = nullptr;
        }
    }

    m_event_delegate = nullptr;

    return true;
}

void sdl_input_interface::update_key_state(int keyIndex, bool down)
{
    if (down)
    {
        if ((m_key_states[keyIndex] & (int)key_state_flags::down) != 0)
        {
            m_key_states[keyIndex] = (int)key_state_flags::down;
        }
        else
        {
            m_key_states[keyIndex] = (int)key_state_flags::down | (int)key_state_flags::pressed;
        }
    }
    else
    {
        if ((m_key_states[keyIndex] & (int)key_state_flags::down) != 0)
        {
            m_key_states[keyIndex] = (int)key_state_flags::released;
        }
        else
        {
            m_key_states[keyIndex] = 0;
        }
    }
}

bool sdl_input_interface::is_window_in_focus()
{
    Uint32 flags = SDL_GetWindowFlags(m_window->get_sdl_handle());
    return (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
}

void sdl_input_interface::pump_events()
{
    profile_marker(profile_colors::system, "sdl_input_interface::pump_events");

    if (!is_window_in_focus())
    {
        return;
    }

    m_current_input = m_pending_input;
    m_mouse_wheel_horizontal = m_pending_mouse_wheel_horizontal;
    m_mouse_wheel_vertical = m_pending_mouse_wheel_vertical;

    m_pending_input = "";
    m_pending_mouse_wheel_horizontal = 0;
    m_pending_mouse_wheel_vertical = 0;
    
    m_mouse_state = (uint32_t)SDL_GetMouseState(&m_mouse_x, &m_mouse_y);
    m_keyboard_state = SDL_GetKeyboardState(nullptr);

    for (size_t i = 0; i < (size_t)input_key::mouse_0; i++)
    {
        bool down = (m_keyboard_state[g_input_key_to_scanecode[i]] != 0);
        update_key_state(i, down);
    }

    for (size_t i = (size_t)input_key::mouse_0; i <= (size_t)input_key::mouse_5; i++)
    {
        int bit = i - (int)input_key::mouse_0;
        bool down = (m_mouse_state & (1 << bit)) != 0;

        update_key_state(i, down);
    }
}

bool sdl_input_interface::is_key_down(input_key key)
{
    return (m_key_states[(int)key] & (int)key_state_flags::down) != 0;
}

bool sdl_input_interface::was_key_pressed(input_key key)
{
    return (m_key_states[(int)key] & (int)key_state_flags::pressed) != 0;
}

bool sdl_input_interface::was_key_released(input_key key)
{
    return (m_key_states[(int)key] & (int)key_state_flags::released) != 0;
}

bool sdl_input_interface::is_modifier_down(input_modifier key)
{
    int mod = 0;

    switch (key)
    {
    case input_modifier::shift: mod = KMOD_SHIFT;   break;
    case input_modifier::ctrl:  mod = KMOD_CTRL;    break;
    case input_modifier::alt:   mod = KMOD_ALT;     break;
    case input_modifier::gui:   mod = KMOD_GUI;     break;
    default:                    db_assert(false);   break;
    }

    return (SDL_GetModState() & mod) != 0;
}

std::string sdl_input_interface::get_clipboard_text()
{
    char* data = SDL_GetClipboardText();
    std::string result = data;
    SDL_free(data);

    return result;
}

void sdl_input_interface::set_clipboard_text(const char* text)
{
    SDL_SetClipboardText(text);
}

vector2 sdl_input_interface::get_mouse_position()
{
    return vector2((float)m_mouse_x, (float)m_mouse_y);
}

void sdl_input_interface::set_mouse_position(const vector2& pos)
{
    if (is_window_in_focus())
    {
        SDL_WarpMouseInWindow(m_window->get_sdl_handle(), (int)pos.x, (int)pos.y);
    }
}

float sdl_input_interface::get_mouse_wheel_delta(bool horizontal)
{
    if (horizontal)
    {
        return m_mouse_wheel_horizontal;
    }
    else
    {
        return m_mouse_wheel_vertical;
    }
}

void sdl_input_interface::set_mouse_cursor(input_cursor cursor)
{    
    SDL_Cursor* sdl_cursor = m_mouse_cursors[(int)cursor];
    if (sdl_cursor != nullptr)
    {
        SDL_SetCursor(sdl_cursor);
    }
}

void sdl_input_interface::set_mouse_capture(bool capture)
{
    m_mouse_captured = capture;
    SDL_CaptureMouse(capture ? SDL_TRUE : SDL_FALSE);
}

bool sdl_input_interface::get_mouse_capture()
{
    return m_mouse_captured;
}

void sdl_input_interface::set_mouse_hidden(bool hidden)
{
    SDL_ShowCursor(!hidden);
}

std::string sdl_input_interface::get_input()
{
    return m_current_input;
}

void sdl_input_interface::handle_event(const SDL_Event* event)
{		
    switch (event->type)
	{
	case SDL_TEXTINPUT:
		{
            m_pending_input += event->text.text;
		}
	case SDL_MOUSEWHEEL:
		{
			if (event->wheel.x > 0) m_pending_mouse_wheel_horizontal += 1;
			if (event->wheel.x < 0) m_pending_mouse_wheel_horizontal -= 1;
			if (event->wheel.y > 0) m_pending_mouse_wheel_vertical += 1;
			if (event->wheel.y < 0) m_pending_mouse_wheel_vertical -= 1;
		}
	}
}

}; // namespace ws
