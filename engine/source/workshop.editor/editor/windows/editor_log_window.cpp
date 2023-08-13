// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/windows/editor_log_window.h"
#include "workshop.core/containers/string.h"

#include "thirdparty/imgui/imgui.h"

namespace ws {

log_handler_window::log_handler_window(editor_log_window* window)
    : m_window(window)
{
}

void log_handler_window::write(log_level level, const std::string& message)
{
    m_window->add_log(level, message);
}

editor_log_window::editor_log_window()
{
    m_handler = std::make_unique<log_handler_window>(this);
}

void editor_log_window::add_log(log_level level, const std::string& message)
{
    std::scoped_lock lock(m_logs_mutex);

    log_entry& entry = m_logs.emplace_back();
    entry.level = level;
    entry.fragments = string_split(message, "\xB3");

    while (entry.fragments.size() > k_log_columns)
    {
        std::string last_entry = entry.fragments.back();
        entry.fragments.pop_back();
        entry.fragments.back() += " \xB3 " + last_entry;
    }

    if (m_logs.size() > k_max_log_entries)
    {
        m_logs.erase(m_logs.begin());
    }
}

void editor_log_window::draw()
{
    std::scoped_lock lock(m_logs_mutex);

    if (m_open)
    {
        if (ImGui::Begin(get_window_id(), &m_open))
        {
            ImGui::BeginTable("OutputTable", 5);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.1f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.05f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.05f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.1f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.6f);

            ImGui::TableNextColumn(); ImGui::TableHeader("Time");
            ImGui::TableNextColumn(); ImGui::TableHeader("Memory");
            ImGui::TableNextColumn(); ImGui::TableHeader("Level");
            ImGui::TableNextColumn(); ImGui::TableHeader("Category");
            ImGui::TableNextColumn(); ImGui::TableHeader("Message");

            for (auto iter = m_logs.rbegin(); iter != m_logs.rend(); iter++)
            {
                log_entry& entry = *iter;

                ImGui::TableNextRow();

                ImGui::TableNextColumn(); ImGui::Text("%s", entry.fragments[0].c_str());
                ImGui::TableNextColumn(); ImGui::Text("%s", entry.fragments[1].c_str());
                ImGui::TableNextColumn(); ImGui::Text("%s", entry.fragments[2].c_str());
                ImGui::TableNextColumn(); ImGui::Text("%s", entry.fragments[3].c_str());
                ImGui::TableNextColumn(); ImGui::Text("%s", entry.fragments[4].c_str());
            }

            ImGui::EndTable();
        }
        ImGui::End();
    }
}

const char* editor_log_window::get_window_id()
{
    return "Output";
}

editor_window_layout editor_log_window::get_layout()
{
    return editor_window_layout::bottom;
}

}; // namespace ws
