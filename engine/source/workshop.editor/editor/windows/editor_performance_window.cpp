// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/windows/editor_performance_window.h"
#include "workshop.core/containers/string.h"
#include "workshop.core/memory/memory_tracker.h"
#include "workshop.core/utils/string_formatter.h"
#include "workshop.core/platform/platform.h"

#include "thirdparty/imgui/imgui.h"

namespace ws {

editor_performance_window::editor_performance_window()
{
    m_stats_triangles_rendered = statistics_manager::get().find_or_create_channel("rendering/triangles_rendered");
    m_stats_draw_calls = statistics_manager::get().find_or_create_channel("rendering/draw_calls");
    m_stats_drawn_instances = statistics_manager::get().find_or_create_channel("rendering/drawn_instances");
    m_stats_culled_instances = statistics_manager::get().find_or_create_channel("rendering/culled_instances");
    m_stats_frame_time_render = statistics_manager::get().find_or_create_channel("frame time/render");
    m_stats_frame_time_render_wait = statistics_manager::get().find_or_create_channel("frame time/render wait");
    m_stats_frame_time_present_wait = statistics_manager::get().find_or_create_channel("frame time/present wait");
    m_stats_frame_time_game = statistics_manager::get().find_or_create_channel("frame time/game");
    m_stats_frame_time_gpu = statistics_manager::get().find_or_create_channel("frame time/gpu");
    m_stats_frame_rate = statistics_manager::get().find_or_create_channel("frame rate");
    m_stats_render_bytes_uploaded = statistics_manager::get().find_or_create_channel("render/bytes uploaded");
}

void editor_performance_window::draw()
{
    const size_t k_width = 250;
    const size_t k_padding = 30;

    size_t display_width = (size_t)ImGui::GetIO().DisplaySize.x;
    size_t display_height = (size_t)ImGui::GetIO().DisplaySize.y;

    if (m_open)
    {
        ImGui::SetNextWindowPos(ImVec2((float)(display_width - k_width - k_padding), (float)k_padding), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(k_width, 0), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.85f);

        if (ImGui::Begin(get_window_id(), &m_open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (ImGui::BeginTable("Stats Table", 2))
            {
                double render_wait = m_stats_frame_time_render_wait->current_value() * 1000.0;
                double present_wait = m_stats_frame_time_present_wait->current_value() * 1000.0;

                double game_time = m_stats_frame_time_game->current_value() * 1000.0;
                double render_time = m_stats_frame_time_render->current_value() * 1000.0;
                double gpu_time = m_stats_frame_time_gpu->current_value() * 1000.0;

                double frame_rate = m_stats_frame_rate->average_value();

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("FPS");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.2f", frame_rate);

                ImGui::NewLine();

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Game Time");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.2f ms", game_time - render_wait);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Render Time");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.2f ms", render_time - present_wait);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("GPU Time");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.2f ms", gpu_time);

                ImGui::NewLine();

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Virtual Memory");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.2f mb", get_memory_usage() / (1024.0 * 1024.0f));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Pagefile Memory");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.2f mb", get_pagefile_usage() / (1024.0 * 1024.0f));

                ImGui::NewLine();

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Triangles Rendered");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zi", static_cast<size_t>(m_stats_triangles_rendered->current_value()));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Draw Calls");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zi", static_cast<size_t>(m_stats_draw_calls->current_value()));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Instances Rendered");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zi", static_cast<size_t>(m_stats_drawn_instances->current_value()));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Instances Culled");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zi", static_cast<size_t>(m_stats_culled_instances->current_value()));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Bytes Uploaded");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zi", static_cast<size_t>(m_stats_render_bytes_uploaded->current_value()));

                ImGui::EndTable();
            }
            ImGui::End();
        }
    }
}

const char* editor_performance_window::get_window_id()
{
    return "Performance";
}

editor_window_layout editor_performance_window::get_layout()
{
    return editor_window_layout::overlay;
}

}; // namespace ws
