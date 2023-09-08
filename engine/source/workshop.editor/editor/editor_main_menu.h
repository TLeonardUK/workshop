// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/utils/init_list.h"
#include "workshop.core/utils/frame_time.h"
#include "workshop.core/utils/singleton.h"
#include "workshop.input_interface/input_interface.h"

namespace ws {

// ================================================================================================
//  Handles updating and rawing the editors main menu bar.
// ================================================================================================
class editor_main_menu 
{
public:

    editor_main_menu(input_interface& input);

    // Draws the main menu via imgui.
    void draw();

public:

    // Type of callback that is invoked when option is clicked in the menu.
    using menu_item_callback_t = std::function<void()>;

    enum class menu_item_type
    {
        item,
        seperator,
        custom
    };

    struct menu_item
    {
    public:
        ~menu_item();

    private:
        friend class editor_main_menu;

        menu_item_type type;
        editor_main_menu* menu;
        std::string path;
        std::vector<std::string> fragments;
        menu_item_callback_t callback;
        bool enabled = true;

        std::vector<input_key> shortcut_keys;
        std::string shortcut;
        bool shortcut_was_down = false;

    public:
        void set_text(const std::string& name)
        {
            fragments.back() = name;
        }

        void set_enabled(bool in_enabled)
        {
            enabled = in_enabled;
        }
    };

    // Handles to an menu item added to the menu.
    using menu_item_handle = std::unique_ptr<menu_item>;

    // Adds an item to the menu bar. When clicked the callback will be invoked.
    // Paths are seperator by /'s and can be multiple levels deep, eg.
    //      Windows/Assets/Asset Loading Manager
    // 
    // The handle returned is ref-counted, when the count reduces to zero the
    // option is removed from the menu.
    menu_item_handle add_menu_item(const char* path, menu_item_callback_t callback);
    menu_item_handle add_menu_item(const char* path, const std::vector<input_key>& shortcut, menu_item_callback_t callback);

    // Adds a menu seperator in the parent path
    menu_item_handle add_menu_seperator(const char* path);

    // Adds a custom menu where the callback is invoked whenever its drawn so custom rendering
    // can be doen for any child elements.
    menu_item_handle add_menu_custom(const char* path, menu_item_callback_t callback);

private:
    struct node
    {
        std::string name;
        menu_item* item;
        std::vector<node> children;
    };

    void add_node(node& root, menu_item* child, std::vector<std::string> remaining_fragments);
    void rebuild_tree();

    void draw_node(node& node);
    void check_shortcuts(node& base);

private:
    node m_root;
    input_interface& m_input;

    std::vector<menu_item*> m_active_menu_items;

};

}; // namespace ws
