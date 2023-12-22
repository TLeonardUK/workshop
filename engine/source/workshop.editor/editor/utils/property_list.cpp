// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/utils/property_list.h"

#include "workshop.game_framework/components/transform/transform_component.h"

#include "workshop.engine/ecs/meta_component.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"

#include "thirdparty/imgui/imgui.h"

namespace ws {

property_list::property_list(asset_manager* ass_manager, asset_database& ass_database, engine& in_engine)
    : m_asset_manager(ass_manager)
    , m_asset_database(ass_database)
    , m_engine(in_engine)
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

    if (min_value != 0 || max_value != 0)
    {
        edit_value = std::clamp(edit_value, min_value, max_value);
        if (edit_value == value)
        {
            modified = false;
        }
    }

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

    if (min_value != 0.0f || max_value != 0.0f)
    {
        edit_value = std::clamp(edit_value, min_value, max_value);
        if (edit_value == value)
        {
            modified = false;
        }
    }

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
            ImGui::GetWindowDrawList()->AddImage(texture, thumbnail_min, thumbnail_max, ImVec2(0, 0), ImVec2(1, 1), ImColor(1.0f, 1.0f, 1.0f, 1.0f));
        }
    }

    ImGui::GetWindowDrawList()->AddRect(preview_min, preview_max, frame_color);
}

bool property_list::draw_edit(reflect_field* field, asset_ptr<model>& value)
{
    std::string path = value.get_path();
    if (draw_asset(field, path, "asset_model"))
    {
        value = m_asset_manager->request_asset<model>(path.c_str(), 0);
        return true;
    }
    return false;
}

bool property_list::draw_edit(reflect_field* field, asset_ptr<material>& value)
{
    std::string path = value.get_path();
    if (draw_asset(field, path, "asset_material"))
    {
        value = m_asset_manager->request_asset<material>(path.c_str(), 0);
        return true;
    }
    return false;
}

bool property_list::draw_edit(reflect_field* field, asset_ptr<texture>& value)
{
    std::string path = value.get_path();
    if (draw_asset(field, path, "asset_texture"))
    {
        value = m_asset_manager->request_asset<texture>(path.c_str(), 0);
        return true;
    }
    return false;
}

bool property_list::draw_edit(reflect_field* field, asset_ptr<shader>& value)
{
    std::string path = value.get_path();
    if (draw_asset(field, path, "asset_shader"))
    {
        value = m_asset_manager->request_asset<shader>(path.c_str(), 0);
        return true;
    }
    return false;
}

bool property_list::draw_edit(reflect_field* field, asset_ptr<scene>& value)
{
    std::string path = value.get_path();
    if (draw_asset(field, path, "asset_scene"))
    {
        value = m_asset_manager->request_asset<scene>(path.c_str(), 0);
        return true;
    }
    return false;
}

bool property_list::draw_asset(reflect_field* field, std::string& path, const char* drag_drop_type)
{
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    char buffer[2048];
    strncpy(buffer, path.c_str(), sizeof(buffer));

    bool ret = false;

    ImGui::InputText("", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
    draw_preview(path.c_str());

    if (ImGui::BeginDragDropTarget())
    {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(drag_drop_type, ImGuiDragDropFlags_None);
        if (payload)
        {
            path = std::string((const char*)payload->Data, payload->DataSize);

            on_before_modify.broadcast(field);

            ret = true;
        }
        ImGui::EndDragDropTarget();
    }

    return ret;
}

bool property_list::draw_edit(reflect_field* field, component_ref_base& value)
{
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    object_manager& obj_manager = m_engine.get_default_world().get_object_manager();

    char buffer[2048] = {};
    if (value.handle != null_object)
    {
        meta_component* meta = obj_manager.get_component<meta_component>(value.handle);
        if (meta)
        {
            strncpy(buffer, meta->name.c_str(), sizeof(buffer));
        }
    }

    bool ret = false;

    ImGui::InputText("", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);

    if (ImGui::BeginDragDropTarget())
    {
        bool valid_payload = true;

        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("object", ImGuiDragDropFlags_AcceptPeekOnly);
        if (payload)
        {
            object handle = *static_cast<object*>(payload->Data);

            // Cannot self-reference.
            if (handle == m_context_object)
            {
                valid_payload = false;
            }
            // Make sure target object has a component of the type we want.
            else if (obj_manager.get_component(handle, value.get_type_index()) == nullptr)
            {
                valid_payload = false;
            }
            // If transform component, do not allow setting parent to one of our children.
            else if (value.get_type_index() == typeid(transform_component))
            {
                transform_component* transform = obj_manager.get_component<transform_component>(m_context_object);
                transform_component* new_transform = obj_manager.get_component<transform_component>(handle);
                if (transform != nullptr && new_transform != nullptr)
                {
                    if (new_transform->is_derived_from(obj_manager, transform))
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
                object handle = *static_cast<object*>(payload->Data);

                on_before_modify.broadcast(field);

                value.handle = handle;

                ret = true;
            }
        }

        ImGui::EndDragDropTarget();
    }

    return ret;
}

bool property_list::draw_edit_enum_flags(reflect_field* field, reflect_enum* enumeration, int64_t& value)
{
    std::vector<const reflect_enum::value*> values = enumeration->get_values();

    for (auto& enum_value : values)
    {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

        bool edit_value = (value & enum_value->value) != 0;
        bool modified = ImGui::Checkbox(enum_value->display_name.c_str(), &edit_value);
        if (modified)
        {
            on_before_modify.broadcast(field);

            if (edit_value)
            {
                value = value | enum_value->value;
            }
            else
            {
                value = value & ~enum_value->value;
            }
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", enum_value->description.c_str());
        }
    }

    return false;
}

bool property_list::draw_edit_enum(reflect_field* field, reflect_enum* enumeration, int64_t& value)
{
    std::vector<const reflect_enum::value*> values = enumeration->get_values();

    int selected_index = 0;
    for (size_t i = 0; i < values.size(); i++)
    {
        if (values[i]->value == value)
        {
            selected_index = (int)i;
            break;
        }
    }

    auto get_data = [](void* data, int idx, const char** out_text) {
        std::vector<const reflect_enum::value*>& values = *reinterpret_cast<std::vector<const reflect_enum::value*>*>(data);
        *out_text = values[idx]->display_name.c_str();
        return true;
    };

    ImGui::PushID(field->get_name());
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    bool modified = ImGui::Combo("", &selected_index, get_data, &values, (int)values.size());
    if (modified)
    {
        on_before_modify.broadcast(field);

        value = values[selected_index]->value;
    }
    ImGui::PopID();

    return modified;
}

bool property_list::draw_field(reflect_field* field, uint8_t* field_data, bool container_element)
{
    bool modified = false;

    float min_value = 0.0f;
    float max_value = 0.0f;
    if (reflect_constraint_range* range = field->get_constraint<reflect_constraint_range>())
    {
        min_value = range->get_min();
        max_value = range->get_max();
    }

    if (!container_element && field->get_container_type() == reflect_field_container_type::list)
    {
        reflect_field_container_helper* helper = field->get_container_helper();
        size_t length = helper->size(field_data);

        for (size_t i = 0; i < length; i++)
        {
            modified |= draw_field(field, (uint8_t*)helper->get_data(field_data, i), true);
        }
    }
    else if (field->get_container_type() == reflect_field_container_type::enumeration)
    {
        reflect_enum* enumeration = get_reflect_enum(field->get_enum_type_index());
        if (enumeration == nullptr)
        {
            ImGui::TextDisabled("Unreflected enum for: %s", field->get_name());
        }
        else if (field->get_type_index() != typeid(int))
        {
            ImGui::TextDisabled("Unsupported underlying type for enum for: %s", field->get_name());
        }
        else
        {
            int64_t value = static_cast<int64_t>(*reinterpret_cast<int*>(field_data));

            if (enumeration->has_flag(reflect_enum_flags::flags))
            {
                modified = draw_edit_enum_flags(field, enumeration, value);
            }
            else
            {
                modified = draw_edit_enum(field, enumeration, value);
            }

            *reinterpret_cast<int*>(field_data) = static_cast<int>(value);
        }
    }
    else if (field->get_type_index() == typeid(int))
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
    else if (field->get_type_index() == typeid(asset_ptr<material>))
    {
        modified = draw_edit(field, *reinterpret_cast<asset_ptr<material>*>(field_data));
    }
    else if (field->get_type_index() == typeid(asset_ptr<texture>))
    {
        modified = draw_edit(field, *reinterpret_cast<asset_ptr<texture>*>(field_data));
    }
    else if (field->get_type_index() == typeid(asset_ptr<shader>))
    {
        modified = draw_edit(field, *reinterpret_cast<asset_ptr<shader>*>(field_data));
    }
    else if (field->get_type_index() == typeid(asset_ptr<scene>))
    {
        modified = draw_edit(field, *reinterpret_cast<asset_ptr<scene>*>(field_data));
    }
    else if (field->get_super_type_index() == typeid(component_ref_base))
    {
        modified = draw_edit(field, *reinterpret_cast<component_ref_base*>(field_data));
    }
    else
    {
        ImGui::TextDisabled("Unsupported type for: %s", field->get_name());
    }

    return modified;
}

bool property_list::draw(object context_object, void* context, reflect_class* context_class)
{
    m_context = context;
    m_context_object = context_object;
    m_class = context_class;

    bool any_modified = false;

    std::vector<reflect_field*> fields = m_class->get_fields(true);
    if (fields.empty())
    {
        ImGui::Indent(5.0f);
        ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "No editable fields.");
        ImGui::Unindent(5.0f);
        return false;
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
            ImGui::PushID(field->get_name());

            uint8_t* field_data = reinterpret_cast<uint8_t*>(m_context) + field->get_offset();
            bool modified = draw_field(field, field_data, false);

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
