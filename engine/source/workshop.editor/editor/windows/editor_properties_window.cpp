// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/windows/editor_properties_window.h"
#include "workshop.editor/editor/editor.h"
#include "workshop.core/containers/string.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.assets/asset.h"
#include "workshop.engine/ecs/object_manager.h"
#include "workshop.engine/ecs/meta_component.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.engine/engine/world.h"
#include "workshop.core/reflection/reflect.h"

#include "thirdparty/imgui/imgui.h"

namespace ws {

editor_properties_window::editor_properties_window(editor* in_editor, engine* in_engine)
    : m_engine(in_engine)
    , m_editor(in_editor)
{
}

void editor_properties_window::set_context(object obj)
{
    object_manager& obj_manager = m_engine->get_default_world().get_object_manager();

    m_context = obj;
    m_component_info.clear();

    if (obj != null_object)
    {
        std::vector<component*> components = obj_manager.get_components(obj);
        for (component* comp : components)
        {
            reflect_class* comp_class = get_reflect_class(comp);
            if (comp_class == nullptr)
            {
                continue;
            }

            component_info& info = m_component_info.emplace_back();
            info.name = comp_class->get_display_name();
            info.property_list = std::make_unique<property_list>(comp, comp_class, &m_engine->get_default_world().get_engine().get_asset_manager(), m_engine->get_asset_database());
            info.component = comp;
            info.on_modified_delegate = info.property_list->on_modified.add_shared([this, &obj_manager, comp](reflect_field* field) {
                obj_manager.component_edited(m_context, comp);
            });
        }
    }
}


void editor_properties_window::draw_add_component()
{
    object_manager& obj_manager = m_engine->get_default_world().get_object_manager();

    std::vector<reflect_class*> existing_components;
    for (component_info& info : m_component_info)
    {
        existing_components.push_back(get_reflect_class(info.component));
    }

    if (ImGui::Button("Add Component", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
    {
        ImGui::OpenPopup("AddComponentWindow");
    }
    ImVec2 add_min = ImGui::GetItemRectMin();
    ImVec2 add_max = ImGui::GetItemRectMax();

    ImGui::SetNextWindowPos(ImVec2(add_min.x, add_max.y));
    ImGui::SetNextWindowSize(ImVec2(add_max.x - add_min.x, 0));
    if (ImGui::BeginPopup("AddComponentWindow"))
    {
        std::vector<reflect_class*> potential_classes = get_reflect_derived_classes(typeid(component));
        std::sort(potential_classes.begin(), potential_classes.end(), [](reflect_class* a, reflect_class* b) {
            return strcmp(a->get_display_name(), b->get_display_name()) < 0;
        });

        for (auto& cls : potential_classes)
        {
            if (cls->has_flag(reflect_class_flags::abstract))
            {
                continue;
            }

            if (std::find(existing_components.begin(), existing_components.end(), cls) != existing_components.end())
            {
                continue;
            }

            if (ImGui::MenuItem(cls->get_display_name()))
            {
                obj_manager.add_component(m_context, cls->get_type_index());
                set_context(m_context);
            }
        }

        ImGui::EndPopup();
    }
}

void editor_properties_window::draw()
{
    object_manager& obj_manager = m_engine->get_default_world().get_object_manager();

    // Update the current context object we are showing.
    std::vector<object> selected_objects = m_editor->get_selected_objects();
    if (selected_objects.size() == 1)
    {
        object context_object = selected_objects[0];
        if (context_object != m_context)
        {
            set_context(context_object);
        }
    }
    else
    {
        set_context(null_object);
    }

    // Draw the property list.
    if (m_open)
    {
        if (ImGui::Begin(get_window_id(), &m_open))
        {
            if (m_context != null_object)
            {
                // Construct add-component menu.
                draw_add_component();

                component* destroy_component = nullptr;

                // Draw each components properties.
                for (component_info& info : m_component_info)
                {
                    ImGui::PushID(info.component);

                    ImVec2 draw_cursor_pos = ImGui::GetCursorPos();
                    ImVec2 available_space = ImGui::GetContentRegionAvail();
                    bool is_open = ImGui::CollapsingHeader(info.name.c_str(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_NoAutoOpenOnLog | ImGuiTreeNodeFlags_AllowItemOverlap);
                    ImVec2 end_cursor_pos = ImGui::GetCursorPos();
                    float header_height = (end_cursor_pos.y - draw_cursor_pos.y);

                    ImVec2 button_pos(
                        draw_cursor_pos.x + available_space.x - 15.0f,
                        draw_cursor_pos.y + 3.0f
                    );

                    ImGui::SetCursorPos(button_pos);
                    if (ImGui::SmallButton("X"))
                    {
                        destroy_component = info.component;
                    }
                    ImGui::SetCursorPos(end_cursor_pos);

                    if (is_open)
                    {
                        info.property_list->draw();
                    }

                    ImGui::PopID();
                }

                // Process deferred component deletion.
                if (destroy_component)
                {
                    obj_manager.remove_component(m_context, destroy_component);
                    set_context(m_context);
                }
            }
        }
        ImGui::End();
    }
}

const char* editor_properties_window::get_window_id()
{
    return "Properties";
}

editor_window_layout editor_properties_window::get_layout()
{
    return editor_window_layout::right;
}

}; // namespace ws
