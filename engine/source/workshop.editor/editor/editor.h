// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/utils/init_list.h"
#include "workshop.core/utils/frame_time.h"
#include "workshop.core/utils/singleton.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.editor/editor/editor_main_menu.h"

#include "thirdparty/imgui/imgui.h"

namespace ws {

class editor_main_menu;
class editor_window;

// Describes what parts of the editor UI should be shown.
enum class editor_mode
{
    // Editor is fully open.
    editor,

    // Editor UI is hidden and the game is shown.
    game,
};

// ================================================================================================
//  This is the core editor class, its responsible for owning all the individual components 
//  required to render the editor ui.
// ================================================================================================
class editor 
    : public singleton<editor>
{
public:

    editor(engine& in_engine);
    ~editor();

    // Registers all the steps required to initialize the engine.
    // Interacting with this class without successfully running these steps is undefined.
    void register_init(init_list& list);

    // Advances the state of the engine. This should be called repeatedly in the main loop of the 
    // application. 
    // 
    // Calling this does not guarantee a new render frame, or the state of the simulation being advanced, 
    // as the engine runs entirely asyncronously. This function mearly provides the opportunity for the engine
    // to advance if its in a state that permits it.
    void step(const frame_time& time);

    // Switches to the given editor mode.
    void set_editor_mode(editor_mode mode);

    // Gets the main menu bar.
    editor_main_menu& get_main_menu();

    // Creates a window and returns a reference to it. The window is owned by the editor.
    template <typename window_type, typename... arg_types>
    window_type& create_window(arg_types... args)
    {
        std::unique_ptr<editor_window> window = std::make_unique<window_type>(args...);
        window_type& result = static_cast<window_type&>(*window.get());
        m_windows.push_back(std::move(window));
        return result;
    }

protected:

    result<void> create_main_menu(init_list& list);
    result<void> destroy_main_menu();

    result<void> create_windows(init_list& list);
    result<void> destroy_windows();

    void draw_dockspace();

    void reset_dockspace_layout();

protected:

    engine& m_engine;

    editor_mode m_editor_mode = editor_mode::game;

    std::unique_ptr<editor_main_menu> m_main_menu;

    std::vector<editor_main_menu::menu_item_handle> m_main_menu_options;

    std::vector<std::unique_ptr<editor_window>> m_windows;

    ImGuiID m_dockspace_id;
    bool m_set_default_dock_space = false;

};

}; // namespace ws
