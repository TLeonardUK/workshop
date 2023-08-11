// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.debug_menu/debug_menu.h"
#include "workshop.input_interface/input_interface.h"
#include "workshop.renderer/render_imgui_manager.h"
#include "workshop.renderer/renderer.h"

namespace ws {

debug_menu::debug_menu()
{
}

debug_menu::option::~option()
{
    auto iter = std::find(menu->m_alive_options.begin(), menu->m_alive_options.end(), this);
    if (iter != menu->m_alive_options.end())
    {
        menu->m_alive_options.erase(iter);
    }
}

void debug_menu::register_init(init_list& list)
{
}

debug_menu::option_handle debug_menu::add_option(const char* path, option_callback_t callback)
{
    option_handle handle = std::make_unique<option>();
    handle->menu = this;
    handle->path = path;
    handle->callback = callback;
    handle->fragments = string_split(path, "/");

    m_alive_options.push_back(handle.get());

    rebuild_tree();

    return handle;
}

void debug_menu::add_node(node& root, option* child, std::vector<std::string> remaining_fragments)
{
    // We need to be added as a leaf to this root.
    if (remaining_fragments.size() == 1)
    {
        node& leaf = root.children.emplace_back();
        leaf.name = remaining_fragments[0];
        leaf.self_option = child;
    }
    // Else find or construct the next node.
    else
    {
        std::string next_fragment = remaining_fragments[0];
        
        node* parent = nullptr;
        auto iter = std::find_if(root.children.begin(), root.children.end(), [&next_fragment](node& sub) {
            return sub.name == next_fragment;
        });

        if (iter != root.children.end())
        {
            parent = &(*iter);
        }
        else
        {
            node& leaf = root.children.emplace_back();
            leaf.name = next_fragment;
            leaf.self_option = nullptr;
            parent = &leaf;
        }

        remaining_fragments.erase(remaining_fragments.begin());
        add_node(*parent, child, remaining_fragments);
    }    
}

void debug_menu::rebuild_tree()
{
    m_root = {};

    for (option* child : m_alive_options)
    {
        add_node(m_root, child, child->fragments);
    }
}

void debug_menu::draw_node(node& base)
{
    if (base.children.size() == 0)
    {
        if (ImGui::MenuItem(base.name.c_str()))
        {
            if (base.self_option)
            {
                base.self_option->callback();
            }
        }
    }
    else
    {
        if (ImGui::BeginMenu(base.name.c_str()))
        {
            for (node& child : base.children)
            {
                draw_node(child);
            }

            ImGui::EndMenu();
        }
    }
}

}; // namespace ws
