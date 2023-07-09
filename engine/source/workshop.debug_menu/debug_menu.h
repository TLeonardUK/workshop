// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"
#include "workshop.core/utils/frame_time.h"

namespace ws {

class input_interface;
class renderer;

// ================================================================================================
//  This renders a imgui menu used for controlling various debugging settings.
// ================================================================================================
class debug_menu
{
public:
    debug_menu();
    
    void register_init(init_list& list);
    void step(const frame_time& time);

    void set_renderer(renderer& renderer);
    void set_input(input_interface& input);

public:

    // Type of callback that is invoked when option is clicked in the menu.
    using option_callback_t = std::function<void()>;

    struct option
    {
        ~option();

        debug_menu* menu;
        std::string path;
        std::vector<std::string> fragments;
        option_callback_t callback;        
    };

    // Handles to an option added to the menu.
    using option_handle = std::unique_ptr<option>;

    // Adds an option to the debug menu bar. When clicked the callback will be invoked.
    // Paths are seperator by /'s and can be multiple levels deep, eg.
    //      Windows/Assets/Asset Loading Manager
    // 
    // The handle returned is ref-counted, when the count reduces to zero the
    // option is removed from the menu.
    option_handle add_option(const char* path, option_callback_t callback);

private:
    struct node
    {
        std::string name;
        option* self_option;
        std::vector<node> children;
    };

    void add_node(node& root, option* child, std::vector<std::string> remaining_fragments);
    void rebuild_tree();

    void draw_node(node& node);

private:
    input_interface* m_input = nullptr;
    renderer* m_renderer = nullptr;

    bool m_is_active = false;

    std::vector<option*> m_alive_options;

    node m_root;

};

}; // namespace ws
