// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/windows/editor_scene_tree_window.h"
#include "workshop.editor/editor/editor.h"
#include "workshop.core/containers/string.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.assets/asset.h"
#include "workshop.engine/ecs/object_manager.h"
#include "workshop.engine/ecs/meta_component.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.engine/engine/world.h"
#include "workshop.game_framework/components/transform/transform_component.h"

#include "thirdparty/imgui/imgui.h"

namespace ws {

editor_scene_tree_window::editor_scene_tree_window(editor* in_editor, world* in_world)
    : m_world(in_world)
    , m_editor(in_editor)
{
}

void editor_scene_tree_window::draw_object_node(object obj, transform_component* transform, size_t depth)
{
    object_manager& obj_manager = m_world->get_object_manager();
    std::vector<object> selected_objects = m_editor->get_selected_objects();

    bool has_children = (transform != nullptr && !transform->children.empty());
    bool draw_children = has_children;
    float indent = (depth * 23.0f) + 0.01f;
    if (!has_children)
    {
        indent += 23.0f;
    }

    meta_component* obj_meta = obj_manager.get_component<meta_component>(obj);

    ImGui::TableNextRow();

    // Name
    ImGui::TableNextColumn();

    ImGui::Indent(indent);
    bool selected = (std::find(selected_objects.begin(), selected_objects.end(), obj) != selected_objects.end());
    bool was_selected = selected;

    ImGui::PushID(string_format("%s/%zi", obj_meta->name.c_str(), obj).c_str());

    // Add drop down button.
    if (has_children)
    {
        auto expanded_iter = std::find(m_expanded_objects.begin(), m_expanded_objects.end(), obj);
        bool expanded = (expanded_iter != m_expanded_objects.end());
        draw_children = expanded;

        if (ImGui::SmallButton(expanded ? "<" : ">"))
        {
            if (expanded)
            {
                m_expanded_objects.erase(expanded_iter);
            }
            else
            {
                m_expanded_objects.push_back(obj);
            }

            m_clicked_item = true;
        }

        ImGui::SameLine();
    }

    // Add background selectable region which we will draw over.
    ImVec2 draw_cursor_pos = ImGui::GetCursorPos();
    ImGui::Selectable("##Selectable", &selected, ImGuiSelectableFlags_None, ImVec2(0, 0));
    ImVec2 end_cursor_pos = ImGui::GetCursorPos();

    // Draw text over selected region.
    ImGui::SetCursorPos(draw_cursor_pos);
    ImGui::Text("%s", obj_meta->name.c_str());
    ImGui::SetCursorPos(end_cursor_pos);

    if (selected != was_selected)
    {
        std::vector<object> new_selection;

        m_clicked_item = true;

        // If ctrl is not held we are doing single select.
        if ((ImGui::GetIO().KeyMods & ImGuiKeyModFlags_Shift) == 0)
        {
            // Select only the current object.
            if (selected)
            {
                new_selection = { obj };
            }
            // Blank out the selection.
            else
            {
                // Nothing required.
            }
        }
        // If ctrl is held we are doing multi-select.
        else
        {
            new_selection = selected_objects;

            // Add to selection.
            if (selected)
            {
                new_selection.push_back(obj);
            }
            // Remove from selection.
            else
            {
                auto iter = std::find(new_selection.begin(), new_selection.end(), obj);
                new_selection.erase(iter);
            }
        }

        m_editor->set_selected_objects(new_selection);
    }
    ImGui::Unindent(indent);

    // Actions
    ImGui::TableNextColumn();
    if (ImGui::SmallButton("X"))
    {
        m_pending_delete = obj;
        m_clicked_item = true;
    }

    ImGui::PopID();

    // Draw children.
    if (draw_children)
    {
        for (auto& ref : transform->children)
        {
            draw_object_node(ref.get_object(), ref.get(&obj_manager), depth + 1);
        }
    }
}

void editor_scene_tree_window::draw()
{ 
    if (m_open)
    {
        if (ImGui::Begin(get_window_id(), &m_open))
        {
            object_manager& obj_manager = m_world->get_object_manager();

            component_filter<const transform_component> transform_filter(obj_manager);
            component_filter<excludes<const transform_component>> no_transform_filter(obj_manager);

            ImGui::BeginChild("ObjectTableView");
            ImGui::BeginTable("ObjectTable", 2);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 16.0f);

            ImGui::TableNextColumn(); ImGui::TableHeader("Name");
            ImGui::TableNextColumn(); ImGui::TableHeader("");

            m_clicked_item = false;
            m_pending_delete = null_object;

            // Draw all the root elements in the tree.
            for (size_t i = 0; i < transform_filter.size(); i++)
            {
                object obj = transform_filter.get_object(i);
                transform_component* transform = transform_filter.get_component<transform_component>(i);

                if (!transform->parent.is_valid(&obj_manager))
                {
                    draw_object_node(obj, transform, 0);
                }
            }
            for (size_t i = 0; i < no_transform_filter.size(); i++)
            {
                object obj = no_transform_filter.get_object(i);
                draw_object_node(obj, nullptr, 0);
            }

            ImGui::EndTable();
            ImGui::EndChild();

            // If we clicked somewhere on this table but didn't click a button, then deselect everything.
            if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                if (!m_clicked_item)
                {
                    std::vector<object> objs = {};
                    m_editor->set_selected_objects(objs);
                }
            }

            // Delete object if requested.
            if (m_pending_delete != null_object)
            {
                obj_manager.destroy_object(m_pending_delete);
            }
        }
        ImGui::End();
    }
}

const char* editor_scene_tree_window::get_window_id()
{
    return "Scene tree";
}

editor_window_layout editor_scene_tree_window::get_layout()
{
    return editor_window_layout::left;
}

}; // namespace ws
