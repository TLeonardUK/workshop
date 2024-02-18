// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/windows/popups/editor_import_asset_popup.h"
#include "workshop.editor/editor/editor.h"
#include "workshop.core/containers/string.h"

#include "thirdparty/imgui/imgui.h"

namespace ws {

editor_import_asset_popup::editor_import_asset_popup(engine* in_engine)
    : m_engine(in_engine)
{
    m_open = false;

    m_property_list = std::make_unique<property_list>(&m_engine->get_asset_manager(), m_engine->get_asset_database(), *m_engine);
}

void editor_import_asset_popup::set_import_settings(asset_importer* importer, const std::string& path, const std::string& output_path)
{
    m_importer = importer;
    m_import_settings = importer->create_import_settings();
    m_path = path;
    m_output_path = output_path;
}

void editor_import_asset_popup::draw()
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

        ImGui::SetNextWindowSize(ImVec2(700.0f, 0.0f), ImGuiCond_Appearing);
        if (ImGui::BeginPopupModal(get_window_id(), nullptr, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove))
        {
            if (!m_open)
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::Text("Import Path:"); ImGui::SameLine(); ImGui::TextDisabled("%s", m_path.c_str());
            ImGui::Text("Output Path:"); ImGui::SameLine(); ImGui::TextDisabled("%s", m_output_path.c_str());

            ImGui::Dummy(ImVec2(0, 10));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 10));

            if (m_import_settings)
            {
                reflect_class* settings_class = get_reflect_class(typeid(*m_import_settings));
                m_property_list->draw(0, m_import_settings.get(), settings_class);
            }

            ImGui::Dummy(ImVec2(0, 10));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 10));

            if (ImGui::Button("Import Asset", ImVec2(150.0f, 0)))
            {
                if (!m_importer->import(m_path.c_str(), m_output_path.c_str(), *m_import_settings))
                {
                    message_dialog("Failed to import asset, view output log for details.", message_dialog_type::error);
                }
                close();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(150.0f, 0)))
            {
                close();
            }

            ImGui::EndPopup();
        }

       ImGui::PopStyleVar();
    }

    m_was_open = m_open;
}

const char* editor_import_asset_popup::get_window_id()
{
    return "Import Asset";
}

editor_window_layout editor_import_asset_popup::get_layout()
{
    return editor_window_layout::popup;
}

}; // namespace ws
