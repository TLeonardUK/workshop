// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/windows/editor_texture_streaming_window.h"
#include "workshop.core/containers/string.h"
#include "workshop.core/filesystem/async_io_manager.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_texture_streamer.h"
#include "workshop.renderer/assets/texture/texture.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.assets/asset.h"

#include "thirdparty/imgui/imgui.h"

namespace ws {

editor_texture_streaming_window::editor_texture_streaming_window(renderer* render)
    : m_renderer(render)
{
}

void editor_texture_streaming_window::draw()
{
    render_texture_streamer& streamer = m_renderer->get_texture_streamer();
    const render_options& options = m_renderer->get_options();

    if (m_open)
    {
        if (ImGui::Begin(get_window_id(), &m_open))
        {
            constexpr size_t load_state_item_count = static_cast<int>(render_texture_streamer::texture_state::COUNT) + 1;
            const char* load_state_items[load_state_item_count];

            load_state_items[0] = "all";
            for (size_t i = 0; i < load_state_item_count - 1; i++)
            {
                load_state_items[i + 1] = render_texture_streamer::texture_state_strings[i];
            }

            ImGui::Text("State");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200.0f);
            ImGui::PushID("LoadState");
            ImGui::Combo("", &m_state_filter, load_state_items, load_state_item_count);
            ImGui::PopID();

            ImGui::SameLine();
            ImGui::Text("Filter");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200.0f);
            ImGui::PushID("Filter");
            ImGui::InputText("", m_filter_buffer, sizeof(m_filter_buffer));
            ImGui::PopID();

            ImGui::SameLine();
            ImGui::Text("IO Bandwidth: %.2f mb/s", async_io_manager::get().get_current_bandwidth() / (1824.0f * 1024.0f));

            // Draw progress bar showing how full the texture pool is.
            float pool_usage_mb = streamer.get_memory_pressure() / (1024.0f * 1024.0f);
            float pool_size_mb = options.texture_streaming_pool_size / (1024.0f * 1024.0f);

            char buffer[256];
            snprintf(buffer, sizeof(buffer), "%.2f mb / %.2f mb", pool_usage_mb, pool_size_mb);
            ImGui::ProgressBar(pool_usage_mb / pool_size_mb, ImVec2(-FLT_MIN, 0), buffer);

            // Draw table showing how many textures for each mip.
            static const size_t k_mip_count = 12;
            std::array<size_t, k_mip_count> mip_counts = { 0 };

            streamer.visit_textures([&mip_counts](const render_texture_streamer::texture_streaming_info& state) mutable {
                if (state.current_resident_mips < mip_counts.size())
                {
                    mip_counts[state.current_resident_mips]++;
                }
            });

            if (ImGui::BeginTable("Mip table", k_mip_count + 1, ImGuiTableFlags_Resizable))
            {
                for (size_t i = 0; i < k_mip_count + 1; i++)
                {
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 1.0f / (k_mip_count + 1));
                }

                ImGui::TableNextColumn(); ImGui::TableHeader("Mip");
                for (size_t i = 0; i < k_mip_count; i++)
                {
                    ImGui::TableNextColumn(); ImGui::TableHeader(string_format("%zi", (size_t)std::pow(2, i + 1)).c_str());
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Count");

                for (size_t i = 0; i < k_mip_count; i++)
                {
                    ImGui::TableNextColumn(); ImGui::Text("%zi", mip_counts[i]);
                }

                ImGui::EndTable();
            }

            // Draw table showing state of every texture.
            ImGui::BeginChild("OutputTableView");
            if (ImGui::BeginTable("OutputTable", 4, ImGuiTableFlags_Resizable))
            {
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.133f);
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.133f);
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.133f);
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.6f);

                ImGui::TableNextColumn(); ImGui::TableHeader("State");
                ImGui::TableNextColumn(); ImGui::TableHeader("Current Mip");
                ImGui::TableNextColumn(); ImGui::TableHeader("Ideal Mip");
                ImGui::TableNextColumn(); ImGui::TableHeader("Texture");

                streamer.visit_textures([this](const render_texture_streamer::texture_streaming_info& state) {

                    if (m_state_filter != 0 && (size_t)state.state != (m_state_filter - 1))
                    {
                        return;
                    }

                    if (*m_filter_buffer != '\0' && state.instance->name.find(m_filter_buffer) == std::string::npos)
                    {
                        return;
                    }

                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", render_texture_streamer::texture_state_strings[(int)state.state]);

                    ImGui::TableNextColumn();
                    ImGui::Text("%i", state.current_resident_mips);

                    ImGui::TableNextColumn();
                    ImGui::Text("%zi", state.ideal_resident_mips);

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", state.instance->name.c_str());

                });

                ImGui::EndTable();
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }
}

const char* editor_texture_streaming_window::get_window_id()
{
    return "Texture Streaming";
}

editor_window_layout editor_texture_streaming_window::get_layout()
{
    return editor_window_layout::bottom_left;
}

}; // namespace ws
