// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/utils/init_list.h"
#include "workshop.core/utils/frame_time.h"
#include "workshop.core/utils/singleton.h"
#include "workshop.core/math/obb.h"
#include "workshop.core/async/async.h"
#include "workshop.core/statistics/statistics_manager.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.engine/assets/scene/scene.h"
#include "workshop.editor/editor/editor_main_menu.h"
#include "workshop.editor/editor/editor_undo_stack.h"
#include "workshop.editor/editor/editor_clipboard.h"
#include "workshop.engine/ecs/object.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/ImGuizmo/ImGuizmo.h"

#include <future>

namespace ws {

class editor_main_menu;
class editor_window;
class camera_component;

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

    template <typename window_type>
    window_type* get_window()
    {
        for (auto& window : m_windows)
        {
            if (window_type* typed = dynamic_cast<window_type*>(window.get()))
            {
                return typed;
            }
        }
        return nullptr;
    }

    // Gets or sets the selected objects.
    std::vector<object> get_selected_objects();
    void set_selected_objects(const std::vector<object>& objects);

    // Gets the main camera used by the editor.
    camera_component* get_camera();

    // Gets the stack used for tracking undo/redo of transactions. All changes
    // to the scene from the editor should go through this.
    editor_undo_stack& get_undo_stack();

protected:
    friend class editor_transaction_change_selected_objects;

    // Sets the selected objects without creating a transaction to do it.
    void set_selected_objects_untransacted(const std::vector<object>& object);

    result<void> create_main_menu(init_list& list);
    result<void> destroy_main_menu();

    void update_main_menu();

    result<void> create_windows(init_list& list);
    result<void> destroy_windows();

    result<void> create_world(init_list& list);
    result<void> destroy_world();

    void draw_selection();
    void draw_dockspace();
    void draw_viewport_toolbar();

    void update_object_picking(bool mouse_over_viewport);

    void reset_dockspace_layout();

    void reset_scene_state();

    void new_scene();
    void open_scene();
    void save_scene(bool ask_for_filename);

    void commit_scene_load();
    void commit_scene_save();
    void process_pending_save_load();

    void cut();
    void copy();
    void paste();

    // Recurses through an object transform tree and adds all objects into the tree into the output vector.
    void gather_sub_tree(object base, std::vector<object>& output);

    void dump_debug_info(const char* tag);

protected:

    engine& m_engine;

    editor_mode m_editor_mode = editor_mode::game;

    // Window State

    std::unique_ptr<editor_main_menu> m_main_menu;
    std::vector<editor_main_menu::menu_item_handle> m_main_menu_options;
    std::vector<std::unique_ptr<editor_window>> m_windows;

    editor_main_menu::menu_item_handle m_undo_menu_item;
    editor_main_menu::menu_item_handle m_redo_menu_item;
    editor_main_menu::menu_item_handle m_cut_menu_item;
    editor_main_menu::menu_item_handle m_copy_menu_item;
    editor_main_menu::menu_item_handle m_paste_menu_item;

    ImGuiID m_dockspace_id = 0;
    bool m_set_default_dock_space = false;

    // Object selection
    std::future<object> m_pick_object_query;
    bool m_pick_object_add_to_selected = false;

    // Scene State

    struct object_state
    {
        vector3 original_scale;
        vector3 original_location;
        quat original_rotation;
    };

    std::vector<object> m_selected_objects;
    std::vector<object_state> m_selected_object_states;

    std::unique_ptr<editor_clipboard> m_clipboard;
    std::unique_ptr<editor_undo_stack> m_undo_stack;

    // Gizmo handling

    vector3 m_pivot_point = vector3::zero;
    ImGuizmo::OPERATION m_current_gizmo_mode = ImGuizmo::OPERATION::TRANSLATE;

    bool m_was_transform_objects = false;

    // Save / Load

    std::string m_current_scene_path = "";
    asset_ptr<scene> m_pending_open_scene;

    bool m_pending_save_scene_success = false;
    task_handle m_pending_save_scene;

    // Snap options.

    float k_translation_snap_options[8] = {
        1.0f,
        10.0f,
        100.0f,
        250.0f,
        500.0f,
        750.0f,
        1000.0f,
        10000.0f
    };

    float k_rotation_snap_options[8] = {
        1.0f,
        10.0f,
        20.0f,
        45.0f,
        60.0f,
        72.0f,
        90.0f,
        120.0f
    };

    float k_scale_snap_options[8] = {
        0.001f,
        0.01f,
        0.1f,
        1.0f,
        2.0f,
        4.0f,
        8.0f,
        16.0f,
    };

    float m_translate_snap = 100.0f;
    float m_rotation_snap = 10.0f;
    float m_scale_snap = 0.1f;

    // Statistics

    statistics_channel* m_stats_frame_rate;

};

}; // namespace ws
