// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/windows/editor_properties_window.h"
#include "workshop.editor/editor/transactions/editor_transaction_create_component.h"
#include "workshop.editor/editor/transactions/editor_transaction_delete_component.h"
#include "workshop.editor/editor/transactions/editor_transaction_modify_component.h"
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
    m_property_list = std::make_unique<property_list>(&m_engine->get_asset_manager(), m_engine->get_asset_database(), *m_engine);
    m_property_list->on_before_modify.add([this](reflect_field* field) {

        if (m_pending_modifications)
        {
            return;
        }

        // Before we make any modifications, serialize the state of the component so we can undo the changes if needed.

        object_manager& obj_manager = m_engine->get_default_world().get_object_manager();
        m_before_modification_component = obj_manager.serialize_component(m_property_list_object, typeid(*m_property_list_component));
        m_pending_modifications = true;
        m_pending_modifications_object = m_property_list_object;
        m_pending_modifications_component = m_property_list_component;

    });
}

void editor_properties_window::draw_add_component(object context)
{
    object_manager& obj_manager = m_engine->get_default_world().get_object_manager();

    std::vector<reflect_class*> existing_components;
    if (context != null_object)
    {
        for (component* comp : obj_manager.get_components(context))
        {
            existing_components.push_back(get_reflect_class(typeid(*comp)));
        }
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
                obj_manager.add_component(context, cls->get_type_index());
                m_editor->get_undo_stack().push(std::make_unique<editor_transaction_create_component>(*m_engine, *m_editor, context, cls->get_type_index()));
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

    object context = null_object;
    if (selected_objects.size() == 1)
    {
        context = selected_objects[0];
    }

    // Draw the property list.
    if (m_open)
    {
        if (ImGui::Begin(get_window_id(), &m_open))
        {
            if (context != null_object)
            {
                // Construct add-component menu.
                draw_add_component(context);

                component* destroy_component = nullptr;

                std::vector<component*> components = obj_manager.get_components(context);

                // Draw each components properties.
                for (component* component : components)
                {
                    ImGui::PushID(component);

                    reflect_class* component_class = get_reflect_class(typeid(*component));

                    ImVec2 draw_cursor_pos = ImGui::GetCursorPos();
                    ImVec2 available_space = ImGui::GetContentRegionAvail();
                    bool is_open = ImGui::CollapsingHeader(component_class->get_display_name(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_NoAutoOpenOnLog | ImGuiTreeNodeFlags_AllowItemOverlap);
                    ImVec2 end_cursor_pos = ImGui::GetCursorPos();
                    float header_height = (end_cursor_pos.y - draw_cursor_pos.y);

                    ImVec2 button_pos(
                        draw_cursor_pos.x + available_space.x - 15.0f,
                        draw_cursor_pos.y + 3.0f
                    );

                    ImGui::SetCursorPos(button_pos);
                    // Don't allow deleting the meta component as that bad boy is important for the basic functioning of the object system.
                    if (dynamic_cast<meta_component*>(component) == nullptr)
                    {
                        if (ImGui::SmallButton("X"))
                        {
                            destroy_component = component;
                        }
                    }
                    ImGui::SetCursorPos(end_cursor_pos);

                    if (is_open)
                    {
                        m_property_list_object = context;
                        m_property_list_component = component;

                        if (m_property_list->draw(context, component, component_class))
                        {
                            obj_manager.component_edited(context, component);
                        }
                    }

                    ImGui::PopID();
                }

                // Process deferred component deletion.
                if (destroy_component)
                {
                    m_editor->get_undo_stack().push(std::make_unique<editor_transaction_delete_component>(*m_engine, *m_editor, context, typeid(*destroy_component)));
                }

                // Wait until active edit item has finished being used and then create a modify transaction
                // so we can rollback the changes made.
                if (m_pending_modifications && !ImGui::IsAnyItemActive())
                {
                    // Make sure the object and component are still valid before applying the modification, they could have 
                    // been deleted elsewhere between when the modification started and now.
                    if (m_pending_modifications_object == context &&
                        std::find(components.begin(), components.end(), m_pending_modifications_component) != components.end())
                    {
                        std::vector<uint8_t> after_changes = obj_manager.serialize_component(m_property_list_object, typeid(*m_property_list_component));

                        m_editor->get_undo_stack().push(std::make_unique<editor_transaction_modify_component>(
                            *m_engine, 
                            *m_editor, 
                            context, 
                            typeid(*m_pending_modifications_component),
                            m_before_modification_component,
                            after_changes
                        ));
                    }
                    m_pending_modifications = false;
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
