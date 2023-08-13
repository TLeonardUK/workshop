// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/windows/editor_log_window.h"

#include "thirdparty/imgui/imgui.h"

namespace ws {

void editor_log_window::draw()
{
    if (m_open)
    {
        if (ImGui::Begin(get_window_id(), &m_open))
        {
            ImGui::Text("Output goes here ...");
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
