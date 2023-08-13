// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/editor_main_menu.h"

#include "thirdparty/imgui/imgui.h"

namespace ws {

editor_main_menu::menu_item::~menu_item()
{
    auto iter = std::find(menu->m_active_menu_items.begin(), menu->m_active_menu_items.end(), this);
    if (iter != menu->m_active_menu_items.end())
    {
        menu->m_active_menu_items.erase(iter);
    }
}


editor_main_menu::menu_item_handle editor_main_menu::add_menu_item(const char* path, menu_item_callback_t callback)
{
    menu_item_handle handle = std::make_unique<menu_item>();
    handle->type = menu_item_type::item;
    handle->menu = this;
    handle->path = path;
    handle->callback = callback;
    handle->fragments = string_split(path, "/");

    m_active_menu_items.push_back(handle.get());

    rebuild_tree();

    return handle;
}

editor_main_menu::menu_item_handle editor_main_menu::add_menu_seperator(const char* path)
{
    menu_item_handle handle = std::make_unique<menu_item>();
    handle->type = menu_item_type::seperator;
    handle->menu = this;
    handle->path = path;
    handle->fragments = string_split(path, "/");
    handle->fragments.push_back(""); // Empty fragment forces a menu item to be created under its parent.

    m_active_menu_items.push_back(handle.get());

    rebuild_tree();

    return handle;
}

editor_main_menu::menu_item_handle editor_main_menu::add_menu_custom(const char* path, menu_item_callback_t callback)
{
    menu_item_handle handle = std::make_unique<menu_item>();
    handle->type = menu_item_type::custom;
    handle->menu = this;
    handle->path = path;
    handle->callback = callback;
    handle->fragments = string_split(path, "/");

    m_active_menu_items.push_back(handle.get());

    rebuild_tree();

    return handle;
}

void editor_main_menu::add_node(node& root, menu_item* child, std::vector<std::string> remaining_fragments)
{
    // We need to be added as a leaf to this root.
    if (remaining_fragments.size() == 1)
    {
        node& leaf = root.children.emplace_back();
        leaf.name = remaining_fragments[0];
        leaf.item = child;
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
            leaf.item = nullptr;
            parent = &leaf;
        }

        remaining_fragments.erase(remaining_fragments.begin());
        add_node(*parent, child, remaining_fragments);
    }    
}

void editor_main_menu::rebuild_tree()
{
    m_root = {};

    for (menu_item* child : m_active_menu_items)
    {
        add_node(m_root, child, child->fragments);
    }
}

void editor_main_menu::draw_node(node& base)
{
    if (base.children.size() == 0)
    {
        if (base.item->type == menu_item_type::seperator)
        {
            ImGui::Separator();
        }
        else if (base.item->type == menu_item_type::custom)
        {
            base.item->callback();
        }
        else if (ImGui::MenuItem(base.name.c_str()))
        {
            if (base.item)
            {
                base.item->callback();
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

void editor_main_menu::draw()
{
    for (node& child : m_root.children)
    {
        draw_node(child);
    }
}

}; // namespace ws
