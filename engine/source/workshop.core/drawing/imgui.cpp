// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/drawing/imgui.h"
#include "workshop.core/containers/string.h"

#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/filesystem/stream.h"

// IO overrides for imgui.

ImFileHandle ImFileOpen(const char* filename, const char* mode)
{
    std::unique_ptr<ws::stream> result = ws::virtual_file_system::get().open(filename, strstr(mode, "w") != nullptr);
    if (!result)
    {
        return nullptr;
    }

    return result.release();
}

bool ImFileClose(ImFileHandle file)
{
    if (file == nullptr)
    {
        return false;
    }
    
    delete file;
    return true;
}

ImU64 ImFileGetSize(ImFileHandle file)
{
    return file->length();
}

ImU64 ImFileRead(void* data, ImU64 size, ImU64 count, ImFileHandle file)
{
    return file->read(static_cast<char*>(data), size * count);
}

ImU64 ImFileWrite(const void* data, ImU64 size, ImU64 count, ImFileHandle file)
{
    return file->write(static_cast<const char*>(data), size * count);
}

// Extended imgui functionality.

namespace ws {

bool ImGuiToggleButton(const char* label, bool active)
{
    ImVec4 base_color = ImGui::GetStyleColorVec4(ImGuiCol_Button);
    ImVec4 active_color = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);

    ImGui::PushStyleColor(ImGuiCol_Button, active ? active_color : base_color);
    bool pressed = ImGui::Button(label);
    ImGui::PopStyleColor(1);

    return pressed;
}

float ImGuiFloatCombo(const char* label, float current_value, float* values, size_t value_count)
{
    char buffer[64];

    size_t current_index = 0;
    for (size_t i = 0; i < value_count; i++)
    {
        if (values[i] == current_value)
        {
            current_index = i;
        }
    }

    snprintf(buffer, sizeof(buffer), "%s: %.3f", label, current_value);

    ImGui::PushID(label);
    ImGui::SetNextItemWidth(130);
    if (!ImGui::BeginCombo("", buffer, ImGuiComboFlags_None))
    {
        ImGui::PopID();
        return values[current_index];
    }

    for (size_t i = 0; i < value_count; i++)
    {
        ImGui::PushID((int)i);

        snprintf(buffer, sizeof(buffer), "%.3f", values[i]);

        bool item_selected = (i == current_index);
        if (ImGui::Selectable(buffer, item_selected))
        {
            current_index = i;
        }

        if (item_selected)
        {
            ImGui::SetItemDefaultFocus();
        }

        ImGui::PopID();
    }

    ImGui::EndCombo();
    ImGui::PopID();

    return values[current_index];
}


}; // namespace ws