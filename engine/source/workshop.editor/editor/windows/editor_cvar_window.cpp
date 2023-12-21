// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/windows/editor_cvar_window.h"
#include "workshop.core/containers/string.h"
#include "workshop.core/memory/memory_tracker.h"
#include "workshop.core/utils/string_formatter.h"
#include "workshop.core/cvar/cvar_manager.h"
#include "workshop.core/cvar/cvar.h"

#include "thirdparty/imgui/imgui.h"

namespace ws {

editor_cvar_window::editor_cvar_window()
{
}

void editor_cvar_window::draw()
{
    if (m_open)
    {
        if (ImGui::Begin(get_window_id(), &m_open))
        {
            ImGui::Text("Filter");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200.0f);
            ImGui::InputText("##", m_filter_buffer, sizeof(m_filter_buffer));
            ImGui::SameLine();
            if (ImGui::Button("Reset to default"))
            {
                cvar_manager::get().reset_to_default();
            }
            ImGui::SameLine();
            if (ImGui::Button("Save"))
            {
                cvar_manager::get().save();
            }
            ImGui::SameLine();
            if (ImGui::Button("Load"))
            {
                cvar_manager::get().load();
            }

            ImGui::BeginChild("CVarView");
            if (ImGui::BeginTable("CVarTable", 4, ImGuiTableFlags_Resizable))
            {
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.3f);
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.15f);
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.15f);
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.4f);

                ImGui::TableNextColumn(); ImGui::TableHeader("Name");
                ImGui::TableNextColumn(); ImGui::TableHeader("Value");
                ImGui::TableNextColumn(); ImGui::TableHeader("Source");
                ImGui::TableNextColumn(); ImGui::TableHeader("Description");

                for (cvar_base* instance : cvar_manager::get().get_cvars())
                {
                    if (*m_filter_buffer != '\0' && std::string(instance->get_name()).find(m_filter_buffer) == std::string::npos)
                    {
                        continue;
                    }

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); ImGui::Text("%s", instance->get_name());
                    ImGui::TableNextColumn(); 
                    ImGui::PushID(instance->get_name());

                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    if (instance->get_value_type() == typeid(int))
                    {
                        int value = instance->get_int();
                        if (ImGui::InputInt("", &value, 1, 100, instance->has_flag(cvar_flag::read_only) ? ImGuiInputTextFlags_ReadOnly : 0))
                        {
                            instance->set_int(value, cvar_source::set_by_user);
                        }
                    }
                    else if (instance->get_value_type() == typeid(float))
                    {
                        float value = instance->get_float();
                        if (ImGui::InputFloat("", &value, 0.0f, 0.0f, "%.3f", instance->has_flag(cvar_flag::read_only) ? ImGuiInputTextFlags_ReadOnly : 0))
                        {
                            instance->set_float(value, cvar_source::set_by_user);
                        }
                    }
                    else if (instance->get_value_type() == typeid(bool))
                    {
                        bool value = instance->get_bool();

                        if (instance->has_flag(cvar_flag::read_only))
                        {
                            char buffer[2048];
                            snprintf(buffer, sizeof(buffer), "%s", value ? "true" : "false");

                            ImGui::InputText("", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
                        }
                        else
                        {
                            if (ImGui::Checkbox("", &value))
                            {
                                instance->set_bool(value, cvar_source::set_by_user);
                            }
                        }
                    }
                    else if (instance->get_value_type() == typeid(std::string))
                    {
                        std::string value = instance->get_string();

                        char buffer[2048];
                        strcpy_s(buffer, value.c_str());

                        if (ImGui::InputText("", buffer, sizeof(buffer), instance->has_flag(cvar_flag::read_only) ? ImGuiInputTextFlags_ReadOnly : 0))
                        {
                            instance->set_string(buffer, cvar_source::set_by_user);
                        }
                    }

                    ImGui::PopID();
                    ImGui::TableNextColumn(); ImGui::Text("%s", cvar_source_strings[(int)instance->get_source()]);
                    ImGui::TableNextColumn(); ImGui::Text("%s", instance->get_description());
                }

                ImGui::EndTable();
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }
}

const char* editor_cvar_window::get_window_id()
{
    return "Console Variables";
}

editor_window_layout editor_cvar_window::get_layout()
{
    return editor_window_layout::bottom_left;
}

}; // namespace ws
