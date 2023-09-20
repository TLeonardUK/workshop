// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/windows/editor_scene_tree_window.h"
#include "workshop.editor/editor/transactions/editor_transaction_create_objects.h"
#include "workshop.editor/editor/transactions/editor_transaction_delete_objects.h"
#include "workshop.editor/editor/transactions/editor_transaction_modify_component.h"
#include "workshop.editor/editor/editor.h"
#include "workshop.core/containers/string.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.assets/asset.h"
#include "workshop.engine/ecs/object_manager.h"
#include "workshop.engine/ecs/meta_component.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.engine/engine/world.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/transform/bounds_component.h"
#include "workshop.game_framework/systems/transform/transform_system.h"

#include "thirdparty/imgui/imgui.h"

namespace ws {

editor_scene_tree_window::editor_scene_tree_window(editor* in_editor, engine* in_engine)
    : m_engine(in_engine)
    , m_editor(in_editor)
{
}

void editor_scene_tree_window::draw_object_node(object obj, transform_component* transform, size_t depth)
{
    object_manager& obj_manager = m_engine->get_default_world().get_object_manager();
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

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        ImGui::Text("%s", obj_meta->name.c_str());

        ImGui::SetDragDropPayload("object", &obj, sizeof(obj), ImGuiCond_Always);
        ImGui::EndDragDropSource();
    }
    else if (ImGui::BeginDragDropTarget())
    {
        bool valid_payload = true;

        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("object", ImGuiDragDropFlags_AcceptPeekOnly);
        if (payload)
        {
            object handle = *static_cast<const object*>(payload->Data);

            transform_component* transform = obj_manager.get_component<transform_component>(obj);
            transform_component* new_transform = obj_manager.get_component<transform_component>(handle);

            // Make sure both this component and the target have a transform component.
            if (transform == nullptr || new_transform == nullptr)
            {
                valid_payload = false;
            }
            // D not allow setting parent to one of our children.
            else
            {
                if (transform != nullptr && new_transform != nullptr)
                {
                    if (transform->is_derived_from(obj_manager, new_transform))
                    {
                        valid_payload = false;
                    }
                }
            }
        }

        if (valid_payload)
        {
            const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("object", ImGuiDragDropFlags_None);
            if (payload)
            {
                object handle = *static_cast<const object*>(payload->Data);

                // Reparent the object.

                std::vector<uint8_t> before_state = obj_manager.serialize_component(handle, typeid(transform_component));
                obj_manager.get_component<transform_component>(handle)->parent = obj;
                std::vector<uint8_t> after_state = obj_manager.serialize_component(handle, typeid(transform_component));

                m_editor->get_undo_stack().push(std::make_unique<editor_transaction_modify_component>(*m_engine, *m_editor, handle, typeid(transform_component), before_state, after_state));
            }
        }
        ImGui::EndDragDropTarget();
    }

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

void editor_scene_tree_window::add_new_object()
{
    object_manager& obj_manager = m_engine->get_default_world().get_object_manager();
    transform_system* transform_sys = obj_manager.get_system<transform_system>();

    std::vector<object> selected_objects = m_editor->get_selected_objects();
    object parent = null_object;
    if (!selected_objects.empty())
    {
        parent = selected_objects[0];

        // If object has no transform, add one as we need one for parenting them.
        if (!obj_manager.get_component<transform_component>(parent))
        {
            obj_manager.add_component<transform_component>(parent);
        }
        if (!obj_manager.get_component<bounds_component>(parent))
        {
            obj_manager.add_component<bounds_component>(parent);
        }

        // Make sure parent is expanded.
        auto iter = std::find(m_expanded_objects.begin(), m_expanded_objects.end(), parent);
        if (iter == m_expanded_objects.end())
        {
            m_expanded_objects.push_back(parent);
        }
    }

    object new_object = obj_manager.create_object("unnamed object");
    obj_manager.add_component<transform_component>(new_object);
    obj_manager.add_component<bounds_component>(new_object);
    transform_sys->set_parent(new_object, parent);

    std::vector<object> handles = { new_object };
    m_editor->get_undo_stack().push(std::make_unique<editor_transaction_create_objects>(*m_engine, *m_editor, handles));

    selected_objects = { new_object };
    m_editor->set_selected_objects(selected_objects);
}

void editor_scene_tree_window::draw()
{ 
    if (m_open)
    {
        if (ImGui::Begin(get_window_id(), &m_open))
        {
            object_manager& obj_manager = m_engine->get_default_world().get_object_manager();

            component_filter<const transform_component> transform_filter(obj_manager);
            component_filter<excludes<const transform_component>> no_transform_filter(obj_manager);

            if (ImGui::Button("Add Object", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
            {
                ImGui::OpenPopup("AddObjectWindow");
            }
            
            ImVec2 add_min = ImGui::GetItemRectMin();
            ImVec2 add_max = ImGui::GetItemRectMax();

            ImGui::SetNextWindowPos(ImVec2(add_min.x, add_max.y));
            ImGui::SetNextWindowSize(ImVec2(add_max.x - add_min.x, 0));
            if (ImGui::BeginPopup("AddObjectWindow"))
            {
                if (ImGui::MenuItem("Empty Object"))
                {
                    add_new_object();
                }

                /*
                // TODO: Add list of prefabs we can instantiate.
                ImGui::Separator();

                if (ImGui::MenuItem("Prefab: sponza"))
                {
                    // TODO
                }
                */

                ImGui::EndPopup();
            }

            // We have an option at the top of the table for unparenting if the user is dragging an object.
            ImGui::Selectable("root", false, ImGuiSelectableFlags_None, ImVec2(ImGui::GetContentRegionAvail().x, 0));
            if (ImGui::GetDragDropPayload() != nullptr)
            {
                if (ImGui::BeginDragDropTarget())
                {
                    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("object", ImGuiDragDropFlags_None);
                    if (payload)
                    {
                        object handle = *static_cast<const object*>(payload->Data);

                        // Unparent the object.

                        std::vector<uint8_t> before_state = obj_manager.serialize_component(handle, typeid(transform_component));
                        obj_manager.get_component<transform_component>(handle)->parent = null_object;
                        std::vector<uint8_t> after_state = obj_manager.serialize_component(handle, typeid(transform_component));

                        m_editor->get_undo_stack().push(std::make_unique<editor_transaction_modify_component>(*m_engine, *m_editor, handle, typeid(transform_component), before_state, after_state));
                    }
                    ImGui::EndDragDropTarget();
                }
            }
            ImGui::Dummy(ImVec2(0, 1.0f));

            ImGui::BeginChild("ObjectTableView");
            if (ImGui::BeginTable("ObjectTable", 2))
            {
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 16.0f);

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

                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::Separator();
                ImGui::TableNextColumn(); ImGui::Separator();

                for (size_t i = 0; i < no_transform_filter.size(); i++)
                {
                    object obj = no_transform_filter.get_object(i);
                    draw_object_node(obj, nullptr, 0);
                }

                ImGui::EndTable();
            }
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
                // If deleted object is currently selected then remove it from selection.
                std::vector<object> objs = m_editor->get_selected_objects();
                auto iter = std::find(objs.begin(), objs.end(), m_pending_delete);
                if (iter != objs.end())
                {
                    objs.erase(iter);
                    m_editor->set_selected_objects(objs);
                }

                // Add the deletion to the undo stack.
                std::vector<object> handles = { m_pending_delete };
                m_editor->get_undo_stack().push(std::make_unique<editor_transaction_delete_objects>(*m_engine, *m_editor, handles));
                //obj_manager.destroy_object(m_pending_delete);
            }
        }
        ImGui::End();
    }
}

const char* editor_scene_tree_window::get_window_id()
{
    return "Scene Tree";
}

editor_window_layout editor_scene_tree_window::get_layout()
{
    return editor_window_layout::left;
}

}; // namespace ws
