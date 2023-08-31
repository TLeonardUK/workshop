// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/utils/property_list.h"

#include "thirdparty/imgui/imgui.h"

#pragma optimize("", off)

namespace ws {

property_list::property_list(void* obj, reflect_class* reflection_class)
    : m_context(obj)
    , m_class(reflection_class)
{
}

bool property_list::draw_edit(reflect_field* field, int& value, int min_value, int max_value)
{
    ImGuiSliderFlags flags = ImGuiSliderFlags_None;
    float step = 1.0f;
    if ((max_value - min_value) != 0.0f)
    {
        step = ((max_value - min_value) / 50.0f);
        flags = ImGuiSliderFlags_Logarithmic;
    }

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    return ImGui::DragInt("##", &value, step, min_value, max_value, "%d", flags);
}

bool property_list::draw_edit(reflect_field* field, float& value, float min_value, float max_value)
{
    ImGuiSliderFlags flags = ImGuiSliderFlags_None;
    float step = 1.0f;
    if ((max_value - min_value) != 0.0f)
    {
        step = ((max_value - min_value) / 50.0f);
        flags = ImGuiSliderFlags_Logarithmic;
    }

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    return ImGui::DragFloat("##", &value, step, min_value, max_value, "%.2f", flags);
};

bool property_list::draw_edit(reflect_field* field, vector3& value, float min_value, float max_value)
{
    ImGuiSliderFlags flags = ImGuiSliderFlags_None;
    float step = 1.0f;
    if ((max_value - min_value) != 0.0f)
    {
        step = ((max_value - min_value) / 50.0f);
        flags = ImGuiSliderFlags_Logarithmic;
    }

    float values[3] = { value.x, value.y, value.z };
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    bool ret = ImGui::DragFloat3("##", values, step, min_value, max_value, "%.2f", flags);
    value.x = values[0];
    value.y = values[1];
    value.z = values[2];

    return ret;
}

bool property_list::draw_edit(reflect_field* field, quat& value, float min_value, float max_value)
{
    ImGuiSliderFlags flags = ImGuiSliderFlags_None;
    float step = 1.0f;
    if ((max_value - min_value) != 0.0f)
    {
        step = ((max_value - min_value) / 50.0f);
    }

    vector3 euler_angle = value.to_euler();
    float values[3] = { math::degrees(euler_angle.x), math::degrees(euler_angle.y), math::degrees(euler_angle.z) };

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    bool ret = ImGui::DragFloat3("##", values, step, min_value, max_value, "%.2f deg", flags);

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
    
    bool ret = ImGui::ColorEdit4("", values, ImGuiColorEditFlags_NoLabel|ImGuiColorEditFlags_Float|ImGuiColorEditFlags_AlphaBar|ImGuiColorEditFlags_AlphaPreview);

    value.r = values[0];
    value.g = values[1];
    value.b = values[2];
    value.a = values[3];

    return ret;
}

bool property_list::draw_edit(reflect_field* field, std::string& value)
{
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    char buffer[2048];
    strncpy(buffer, value.c_str(), sizeof(buffer));

    bool ret = ImGui::InputText("", buffer, sizeof(buffer));
    value = buffer;

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

    if (ImGui::BeginTable("ObjectTable", 2, ImGuiTableFlags_Resizable| ImGuiTableFlags_BordersInnerV))
    {
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

            float min_value = 0.0f;
            float max_value = 0.0f;
            if (reflect_constraint_range* range = field->get_constraint<reflect_constraint_range>())
            {
                min_value = range->get_min();
                max_value = range->get_max();
            }


            ImGui::PushID(field->get_name());

            if (field->get_type_index() == typeid(int))
            {
                modified = draw_edit(field, *reinterpret_cast<int*>(field_data), static_cast<int>(min_value), static_cast<int>(max_value));
            }
            else if (field->get_type_index() == typeid(size_t))
            {
                int value = (int)*reinterpret_cast<size_t*>(field_data);
                modified = draw_edit(field, value, static_cast<int>(min_value), static_cast<int>(max_value));
                *reinterpret_cast<size_t*>(field_data) = value;
            }
            else if (field->get_type_index() == typeid(float))
            {
                modified = draw_edit(field, *reinterpret_cast<float*>(field_data), min_value, max_value);
            }
            else if (field->get_type_index() == typeid(bool))
            {
                modified = draw_edit(field, *reinterpret_cast<bool*>(field_data));
            }
            else if (field->get_type_index() == typeid(vector3))
            {
                modified = draw_edit(field, *reinterpret_cast<vector3*>(field_data), min_value, max_value);
            }
            else if (field->get_type_index() == typeid(quat))
            {
                modified = draw_edit(field, *reinterpret_cast<quat*>(field_data), min_value, max_value);
            }
            else if (field->get_type_index() == typeid(color))
            {
                modified = draw_edit(field, *reinterpret_cast<color*>(field_data));
            }
            else if (field->get_type_index() == typeid(std::string))
            {
                modified = draw_edit(field, *reinterpret_cast<std::string*>(field_data));
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
}

}; // namespace ws
