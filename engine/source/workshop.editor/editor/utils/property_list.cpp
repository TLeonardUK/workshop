// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/utils/property_list.h"

#include "thirdparty/imgui/imgui.h"

namespace ws {

property_list::property_list(asset_manager* ass_manager, asset_database& ass_database)
    : m_asset_manager(ass_manager)
    , m_asset_database(ass_database)
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

    int edit_value = value;
    bool modified = ImGui::DragInt("##", &edit_value, step, min_value, max_value, "%d", flags);
    if (modified)
    {
        on_before_modify.broadcast(field);
        value = edit_value;
    }

    return modified;
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

    float edit_value = value;
    bool modified = ImGui::DragFloat("##", &edit_value, step, min_value, max_value, "%.2f", flags);
    if (modified)
    {
        on_before_modify.broadcast(field);
        value = edit_value;
    }

    return modified;
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
    bool modified = ImGui::DragFloat3("##", values, step, min_value, max_value, "%.2f", flags);
    if (modified)
    {
        on_before_modify.broadcast(field);
        value.x = values[0];
        value.y = values[1];
        value.z = values[2];
    }
    
    return modified;
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
    bool modified = ImGui::DragFloat3("##", values, step, min_value, max_value, "%.2f deg", flags);
    if (modified)
    {
        on_before_modify.broadcast(field);
        value = quat::euler(vector3(math::radians(values[0]), math::radians(values[1]), math::radians(values[2])));
    }

    return modified;
}

bool property_list::draw_edit(reflect_field* field, bool& value)
{
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    bool edit_value = value;
    bool modified = ImGui::Checkbox("", &edit_value);
    if (modified)
    {
        on_before_modify.broadcast(field);
        value = edit_value;
    }

    return modified;
}

bool property_list::draw_edit(reflect_field* field, color& value)
{
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    float values[4] = { value.r, value.g, value.b, value.a };
    
    bool modified = ImGui::ColorEdit4("", values, ImGuiColorEditFlags_NoLabel|ImGuiColorEditFlags_Float|ImGuiColorEditFlags_AlphaBar|ImGuiColorEditFlags_AlphaPreview);
    if (modified)
    {
        on_before_modify.broadcast(field);
        value.r = values[0];
        value.g = values[1];
        value.b = values[2];
        value.a = values[3];
    }

    return modified;
}

bool property_list::draw_edit(reflect_field* field, std::string& value)
{
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    char buffer[2048];
    strncpy(buffer, value.c_str(), sizeof(buffer));

    bool modified = ImGui::InputText("", buffer, sizeof(buffer));
    if (modified)
    {
        on_before_modify.broadcast(field);
        value = buffer;
    }

    return modified;
}

void property_list::draw_preview(const char* asset_path)
{
    ImColor frame_color = ImGui::GetStyleColorVec4(ImGuiCol_Border);

    ImVec2 preview_min = ImGui::GetCursorScreenPos();
    ImVec2 preview_max = ImVec2(
        preview_min.x + k_preview_size,
        preview_min.y + k_preview_size
    );

    ImVec2 thumbnail_min = ImVec2(
        preview_min.x + k_preview_padding,
        preview_min.y + k_preview_padding
    );
    ImVec2 thumbnail_max = ImVec2(
        preview_max.x - k_preview_padding,
        preview_max.y - k_preview_padding
    );

    ImGui::Dummy(ImVec2(k_preview_size, k_preview_size));
    ImGui::GetWindowDrawList()->AddRectFilled(thumbnail_min, thumbnail_max, ImColor(0.0f, 0.0f, 0.0f, 0.5f));

    asset_database_entry* entry = m_asset_database.get(asset_path);
    if (entry)
    {
        asset_database::thumbnail* thumb = m_asset_database.get_thumbnail(entry);
        if (thumb)
        {
            ImTextureID texture = thumb->thumbnail_texture.get();
            ImGui::GetWindowDrawList()->AddImage(texture, thumbnail_min, thumbnail_max, ImVec2(0, 0), ImVec2(1, 1), ImColor(1.0f, 1.0f, 1.0f, 0.5f));
        }
    }

    ImGui::GetWindowDrawList()->AddRect(preview_min, preview_max, frame_color);
}

bool property_list::draw_edit(reflect_field* field, asset_ptr<model>& value)
{
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    char buffer[2048];
    strncpy(buffer, value.get_path().c_str(), sizeof(buffer));

    bool ret = false;

    ImGui::InputText("", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
    draw_preview(value.get_path().c_str());

    if (ImGui::BeginDragDropTarget())
    {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("asset_model", ImGuiDragDropFlags_None);
        if (payload)
        {
            std::string asset_path((const char*)payload->Data, payload->DataSize);

            on_before_modify.broadcast(field);

            value = m_asset_manager->request_asset<model>(asset_path.c_str(), 0);

            ret = true;
        }
        ImGui::EndDragDropTarget();
    }

    return ret;
}

bool property_list::draw(void* context, reflect_class* context_class)
{
    m_context = context;
    m_class = context_class;

    bool any_modified = false;

    std::vector<reflect_field*> fields = m_class->get_fields(true);

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
            else if (field->get_type_index() == typeid(asset_ptr<model>))
            {
                modified = draw_edit(field, *reinterpret_cast<asset_ptr<model>*>(field_data));
            }
            else
            {
                ImGui::Text("Unsupported Edit Type");
            }

            if (modified)
            {
                on_modified.broadcast(field);
            }

            any_modified |= modified;

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    return any_modified;
}

}; // namespace ws
