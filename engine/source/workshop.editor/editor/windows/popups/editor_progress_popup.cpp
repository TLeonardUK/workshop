// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/windows/popups/editor_progress_popup.h"
#include "workshop.core/containers/string.h"

#include "thirdparty/imgui/imgui.h"

namespace ws {

editor_progress_popup::editor_progress_popup()
{
    m_open = false;
}

void editor_progress_popup::set_title(const char* text)
{
    m_title = text;
}

void editor_progress_popup::set_subtitle(const char* text)
{
    m_subtitle = text;
}

void editor_progress_popup::set_progress(float progress)
{
    m_progress = progress;
}

void editor_progress_popup::draw()
{
    // If we've just been opened then tell imgui to open the popup.
    if (m_open)
    {
        if (!ImGui::IsPopupOpen(get_window_id()))
        {
            ImGui::OpenPopup(get_window_id());
        }
    }

    if (m_open || m_was_open)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 20.0f));

        ImGui::SetNextWindowSize(ImVec2(800.0f, 0.0f), ImGuiCond_Always);
        if (ImGui::BeginPopupModal(get_window_id(), nullptr, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove))
        {
            if (!m_open)
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::Text("%s", m_title.c_str());
            ImGui::Text("%s", m_subtitle.c_str());
            ImGui::Dummy(ImVec2(0.0f, 20.0f));
            ImGui::ProgressBar(m_progress);

            ImGui::EndPopup();
        }

       ImGui::PopStyleVar();
    }

    m_was_open = m_open;
}

const char* editor_progress_popup::get_window_id()
{
    return "Progress";
}

editor_window_layout editor_progress_popup::get_layout()
{
    return editor_window_layout::popup;
}

}; // namespace ws
