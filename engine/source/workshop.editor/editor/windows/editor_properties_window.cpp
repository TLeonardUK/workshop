// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/windows/editor_properties_window.h"
#include "workshop.core/containers/string.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.assets/asset.h"

#include "thirdparty/imgui/imgui.h"

namespace ws {

editor_properties_window::editor_properties_window()
{
}

void editor_properties_window::draw()
{ 
    if (m_open)
    {
        if (ImGui::Begin(get_window_id(), &m_open))
        {
        }
        ImGui::End();
    }
}

const char* editor_properties_window::get_window_id()
{
    return "Properties";
}

editor_window_layout editor_properties_window::get_layout()
{
    return editor_window_layout::right;
}

}; // namespace ws
