// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/utils/property_list.h"

#include "thirdparty/imgui/imgui.h"

namespace ws {

property_list::property_list(void* obj, reflect_class* reflection_class)
    : m_context(obj)
    , m_class(reflection_class)
{
}

bool property_list::draw_edit(reflect_field* field, int& value)
{
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    return ImGui::DragInt("##", &value);
}

bool property_list::draw_edit(reflect_field* field, float& value)
{
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    return ImGui::DragFloat("##", &value);
};

bool property_list::draw_edit(reflect_field* field, vector3& value)
{
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    float values[3] = { value.x, value.y, value.z };
    bool ret = ImGui::DragFloat3("##", values);
    value.x = values[0];
    value.y = values[1];
    value.z = values[2];

    return ret;
}

bool property_list::draw_edit(reflect_field* field, quat& value)
{
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    vector3 euler_angle = value.to_euler();
    float values[3] = { math::degrees(euler_angle.x), math::degrees(euler_angle.y), math::degrees(euler_angle.z) };

    bool ret = ImGui::DragFloat3("##", values);

    value = quat::euler(vector3(math::radians(values[0]), math::radians(values[1]), math::radians(values[2])));

    return ret;
}

bool property_list::draw_edit(reflect_field* field, bool& value)
{
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    return ImGui::Checkbox("", &value);
}

bool property_list::draw_edit(reflect_field* field, color& value)
{
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    float values[4] = { value.r, value.g, value.b, value.a };
    
    bool ret = ImGui::ColorEdit4("", values, ImGuiColorEditFlags_NoLabel);

    value.r = values[0];
    value.g = values[1];
    value.b = values[2];
    value.a = values[3];

    return ret;
}

void property_list::draw()
{
    std::vector<reflect_field*> fields;

    // Grab the fields from the class and all its parents.
    reflect_class* collect_class = m_class;
    while (collect_class)
    {
        std::vector<reflect_field*> class_fields = collect_class->get_fields();
        fields.insert(fields.end(), class_fields.begin(), class_fields.end());
        collect_class = collect_class->get_parent();
    }

    ImGui::BeginTable("ObjectTable", 2, ImGuiTableFlags_Resizable| ImGuiTableFlags_BordersInnerV);
    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.5f);
    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.5f);

    for (reflect_field* field : fields)
    {
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::Text("%s", field->get_display_name());        
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", field->get_description());
        }

        ImGui::TableNextColumn();

        // All the edit types, we should probably abstract these somewhere.
        uint8_t* field_data = reinterpret_cast<uint8_t*>(m_context) + field->get_offset();

        bool modified = false;

        ImGui::PushID(field->get_name());
        if (field->get_type_index() == typeid(int))
        {
            modified = draw_edit(field, *reinterpret_cast<int*>(field_data));
        }
        else if (field->get_type_index() == typeid(size_t))
        {
            int value = (int)*reinterpret_cast<size_t*>(field_data);
            modified = draw_edit(field, value);
            *reinterpret_cast<size_t*>(field_data) = value;
        }
        else if (field->get_type_index() == typeid(float))
        {
            modified = draw_edit(field, *reinterpret_cast<float*>(field_data));
        }
        else if (field->get_type_index() == typeid(bool))
        {
            modified = draw_edit(field, *reinterpret_cast<bool*>(field_data));
        }
        else if (field->get_type_index() == typeid(vector3))
        {
            modified = draw_edit(field, *reinterpret_cast<vector3*>(field_data));
        }
        else if (field->get_type_index() == typeid(quat))
        {
            modified = draw_edit(field, *reinterpret_cast<quat*>(field_data));
        }
        else if (field->get_type_index() == typeid(color))
        {
            modified = draw_edit(field, *reinterpret_cast<color*>(field_data));
        }
        else
        {
            ImGui::Text("Unsupported Edit Type");
        }

        if (modified)
        {
            on_modified.broadcast(field);
        }

        ImGui::PopID();
    }

    ImGui::EndTable();
}

}; // namespace ws
