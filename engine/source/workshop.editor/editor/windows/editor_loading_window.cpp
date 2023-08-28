// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/windows/editor_loading_window.h"
#include "workshop.core/containers/string.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.assets/asset.h"

#include "thirdparty/imgui/imgui.h"

#pragma optimize("", off)

namespace ws {

editor_loading_window::editor_loading_window(asset_manager* ass_manager)
    : m_asset_manager(ass_manager)
{
}

void editor_loading_window::draw()
{ 
    if (m_open)
    {
        if (ImGui::Begin(get_window_id(), &m_open))
        {
            constexpr size_t load_state_item_count = static_cast<int>(asset_loading_state::COUNT) + 1;
            const char* load_state_items[load_state_item_count];

            load_state_items[0] = "all";
            for (size_t i = 0; i < load_state_item_count - 1; i++)
            {
                load_state_items[i + 1] = asset_loading_state_strings[i];
            }

            ImGui::Text("State");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200.0f);
            ImGui::PushID("LoadState");
            ImGui::Combo("", &m_load_state, load_state_items, load_state_item_count);
            ImGui::PopID();

            /*
            ImGui::SameLine();
            ImGui::Text("IO Bandwidth: %.2f MB/s", 2.01f);
            */

            ImGui::BeginChild("OutputTableView");
            ImGui::BeginTable("OutputTable", 4, ImGuiTableFlags_Resizable);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.1f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.1f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.1f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.7f);

            ImGui::TableNextColumn(); ImGui::TableHeader("State");
            ImGui::TableNextColumn(); ImGui::TableHeader("Priority");
            ImGui::TableNextColumn(); ImGui::TableHeader("Time");
            ImGui::TableNextColumn(); ImGui::TableHeader("Asset");

            //Building 1.5s path/path/path/path
            m_asset_manager->visit_assets([this](asset_state* state) {

                if (m_load_state != 0 && (size_t)state->loading_state != (m_load_state - 1))
                {
                    return;
                }

                ImGui::TableNextRow();

                ImGui::TableNextColumn(); 
                ImGui::Text("%s", asset_loading_state_strings[(int)state->loading_state]);

                ImGui::TableNextColumn();
                ImGui::Text("%i", state->priority);

                ImGui::TableNextColumn(); 
                ImGui::Text("%.1f ms", state->load_timer.get_elapsed_ms());
                
                ImGui::TableNextColumn(); 
                ImGui::Text("%s", state->path.c_str());

            });

            ImGui::EndTable();
            ImGui::EndChild();
        }
        ImGui::End();
    }
}

const char* editor_loading_window::get_window_id()
{
    return "Loading";
}

editor_window_layout editor_loading_window::get_layout()
{
    return editor_window_layout::bottom;
}

}; // namespace ws
