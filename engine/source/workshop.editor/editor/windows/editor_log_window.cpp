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

void log_handler_window::write_raw(log_level level, log_source source, const std::string& timestamp, const std::string& message)
{
    m_window->add_log(level, source, timestamp, message);
}

editor_log_window::editor_log_window()
{
    m_handler = std::make_unique<log_handler_window>(this);
    m_base_max_log_level = log_handler::get_max_log_level();
}

void editor_log_window::add_log(log_level level, log_source source, const std::string& timestamp, const std::string& message)
{
    std::scoped_lock lock(m_logs_mutex);

    log_entry& entry = m_logs.emplace_back();
    entry.level = level;
    entry.category = source;
    entry.message = message;
    entry.timestamp = timestamp;
    entry.search_key = string_lower(message);

    if (m_logs.size() > k_max_log_entries)
    {
        m_logs.erase(m_logs.begin());
    }
}

void editor_log_window::apply_filter()
{
    std::string filter = string_lower(m_filter_buffer);

    for (log_entry& entry : m_logs)
    {
        bool valid = true;

        if (m_log_level > 0 && static_cast<int>(entry.level) > (m_log_level - 1))
        {
            valid = false;
        }
        else if (m_log_category > 0 && static_cast<int>(entry.category) != (m_log_category - 1))
        {
            valid = false;
        }
        else if (!filter.empty() &&  entry.search_key.find(filter) == std::string::npos)
        {
            valid = false;
        }

        entry.filtered_out = !valid;
    }

    // If log level is above our current global filter, change it.
    log_handler::set_max_log_level(static_cast<log_level>(std::max((int)m_base_max_log_level, (int)m_log_level - 1)));
}

void editor_log_window::draw()
{
    std::scoped_lock lock(m_logs_mutex);

    if (m_open)
    {
        if (ImGui::Begin(get_window_id(), &m_open))
        {
            constexpr size_t level_item_count = static_cast<int>(log_level::count) + 1;            
            const char* level_items[level_item_count];

            level_items[0] = "all";
            for (size_t i = 0; i < level_item_count - 1; i++)
            {
                level_items[i + 1] = log_level_strings[i];
            }

            constexpr size_t category_item_count = static_cast<int>(log_source::count) + 1;
            const char* category_items[category_item_count];

            category_items[0] = "all";
            for (size_t i = 0; i < category_item_count - 1; i++)
            {
                category_items[i + 1] = log_source_strings[i];
            }

            ImGui::Text("Minimum Level");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200.0f);
            ImGui::PushID("LogLevel");
            if (ImGui::Combo("", &m_log_level, level_items, level_item_count))
            {
                apply_filter();
            }
            ImGui::PopID();

            ImGui::SameLine();
            ImGui::Text("Category");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200.0f);
            ImGui::PushID("LogCategory");
            if (ImGui::Combo("", &m_log_category, category_items, category_item_count))
            {
                apply_filter();
            }
            ImGui::PopID();

            ImGui::SameLine();
            ImGui::Text("Filter");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200.0f);
            ImGui::PushID("Filter");
            if (ImGui::InputText("", m_filter_buffer, sizeof(m_filter_buffer)))
            {
                apply_filter();
            }
            ImGui::PopID();

            ImGui::BeginChild("OutputTableView");
            ImGui::BeginTable("OutputTable", 4, ImGuiTableFlags_Resizable);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.1f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.1f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.1f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.7f);

            ImGui::TableNextColumn(); ImGui::TableHeader("Timestamp");
            ImGui::TableNextColumn(); ImGui::TableHeader("Level");
            ImGui::TableNextColumn(); ImGui::TableHeader("Category");
            ImGui::TableNextColumn(); ImGui::TableHeader("Message");

            for (auto iter = m_logs.rbegin(); iter != m_logs.rend(); iter++)
            {
                log_entry& entry = *iter;
                if (entry.filtered_out)
                {
                    continue;
                }

                ImGui::TableNextRow();

                ImGui::TableNextColumn(); ImGui::Text("%s", entry.timestamp.c_str());
                ImGui::TableNextColumn(); ImGui::Text("%s", log_level_strings[(int)entry.level]);
                ImGui::TableNextColumn(); ImGui::Text("%s", log_source_strings[(int)entry.category]);
                ImGui::TableNextColumn(); ImGui::Text("%s", entry.message.c_str());
            }

            ImGui::EndTable();
            ImGui::EndChild();
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
