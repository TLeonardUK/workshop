// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/windows/editor_assets_window.h"
#include "workshop.core/containers/string.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.assets/asset.h"
#include "workshop.core/platform/platform.h"
#include "workshop.core/filesystem/virtual_file_system.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/imgui_internal.h"

namespace ws {

editor_assets_window::editor_assets_window(asset_manager* ass_manager, asset_database* ass_database)
    : m_asset_manager(ass_manager)
    , m_asset_database(ass_database)
{
}

void editor_assets_window::draw_asset_tree_dir(asset_database_entry* entry)
{
    if (entry == nullptr)
    {
        return;
    }

    std::vector<asset_database_entry*> child_dirs = entry->get_directories();

    ImGuiTreeNodeFlags flags = 0;
    if (child_dirs.empty()) 
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (entry->get_path() == m_selected_path) 
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (ImGui::TreeNodeEx(entry->get_name(), flags))
    {
        if (ImGui::IsItemClicked() || ImGui::IsItemToggledOpen())
        {
            m_selected_path = entry->get_path();
        }

        for (asset_database_entry* child : child_dirs)
        {
            draw_asset_tree_dir(child);
        }

        ImGui::TreePop();
    }
}

void editor_assets_window::draw_asset_tree()
{
    draw_asset_tree_dir(m_asset_database->get("data:/"));
}

void editor_assets_window::draw_asset_list()
{
    if (m_selected_path.empty())
    {
        return;
    }

    asset_database_entry* entry = m_asset_database->get(m_selected_path.c_str());
    if (entry)
    {
        constexpr float k_item_width = 96;
        constexpr float k_item_height = 164;
        constexpr float k_item_padding = 10.0f;
        constexpr float k_preview_padding = 2.0f;

        ImVec2 region = ImGui::GetContentRegionAvail();

        size_t items_per_row = std::max(1, static_cast<int>(region.x / (k_item_width + k_item_padding)));

        ImColor frame_color = ImGui::GetStyleColorVec4(ImGuiCol_Border);

        std::string lowercase_filter = string_lower(m_current_filter);

        // Grab all files with our asset extension.
        std::vector<asset_database_entry*> files = entry->get_files();
        for (auto iter = files.begin(); iter != files.end(); /* empty */)
        {
            asset_database_entry* entry = *iter;
            std::string extension = virtual_file_system::get().get_extension(entry->get_name());

            bool include = true;

            bool metadata_valid = entry->has_metadata();
            if (metadata_valid)
            {
                asset_database_metadata* meta = entry->get_metadata();
                if (m_current_filter_type != 0)
                {
                    if (meta->descriptor_type != m_filter_types[m_current_filter_type])
                    {
                        include = false;
                    }
                }

                if (m_current_filter[0] != '\0')
                {
                    if (strstr(entry->get_filter_key(), lowercase_filter.c_str()) == nullptr)
                    {
                        include = false;
                    }
                }
            }

            if (extension == asset_manager::k_asset_extension && include)
            {
                iter++;
            }
            else
            {
                iter = files.erase(iter);
            }
        }

        // Draw blocks for each asset.
        ImVec2 screen_base_pos = ImGui::GetCursorScreenPos();

        for (size_t i = 0; i < files.size(); i++)
        {
            asset_database_entry* file = files[i];

            ImVec2 item_min = ImVec2(
                static_cast<float>(k_item_padding + ((i % items_per_row) * (k_item_width + k_item_padding))),
                static_cast<float>(k_item_padding + ((i / items_per_row) * (k_item_height + k_item_padding)))
            );
            ImVec2 item_max = ImVec2(
                item_min.x + k_item_width,
                item_min.y + k_item_height
            );
            ImVec2 screen_item_min = ImVec2(
                screen_base_pos.x + item_min.x,
                screen_base_pos.y + item_min.y
            );
            ImVec2 screen_item_max = ImVec2(
                screen_base_pos.x + item_max.x,
                screen_base_pos.y + item_max.y
            );
            ImRect item_bb(item_min, item_max);

            ImGui::PushID(file->get_path());

            ImGui::SetCursorScreenPos(screen_item_min);

            bool selected = (_stricmp(file->get_path(), m_selected_file.c_str()) == 0);
            if (ImGui::Selectable("", &selected, ImGuiSelectableFlags_None, ImVec2(k_item_width, k_item_height)))
            {
                m_selected_file = file->get_path();
            }

            // Skip any drawing if offscreen.
            if (!ImGui::IsItemVisible())
            {
                ImGui::PopID();
                continue;
            }

            bool is_dragging = ImGui::BeginDragDropSource(ImGuiDragDropFlags_None);
            if (is_dragging)
            {
                std::string asset_type = string_format("asset_%s", file->get_metadata()->descriptor_type.c_str());

                ImGui::SetDragDropPayload(asset_type.c_str(), file->get_path(), strlen(file->get_path()), ImGuiCond_Always);

                // Reset item bounds as we are now drawing inside the drag tooltip.
                screen_item_min = ImGui::GetCursorScreenPos();
                screen_item_max.x = screen_item_min.x + k_item_width;
                screen_item_max.y = screen_item_min.y + k_item_height;
                item_min = ImVec2(0.0f, 0.0f);
                item_max = ImVec2(k_item_width, k_item_height);
                item_bb = ImRect(item_min, item_max);

                // Ensure tooltip is the correct size.
                ImGui::Dummy(ImVec2(k_item_width, k_item_height));
            }
            
            ImVec2 preview_min = screen_item_min;
            preview_min.x += k_preview_padding;
            preview_min.y += k_preview_padding;

            ImVec2 preview_max = ImVec2(screen_item_min.x + k_item_width, screen_item_min.y + k_item_width);
            preview_max.x -= k_preview_padding;
            preview_max.y -= k_preview_padding;

            asset_database_metadata* meta = file->get_metadata();
            asset_database::thumbnail* thumb = m_asset_database->get_thumbnail(file);
            if (thumb)
            {
                ImTextureID texture = thumb->thumbnail_texture.get();
                ImGui::GetWindowDrawList()->AddImage(texture, preview_min, preview_max, ImVec2(0, 0), ImVec2(1, 1), ImColor(1.0f, 1.0f, 1.0f, 0.5f));
            }
            else
            {
                ImGui::GetWindowDrawList()->AddRectFilled(preview_min, preview_max, ImColor(0.0f, 0.0f, 0.0f, 0.5f));
            }

            ImGui::GetWindowDrawList()->AddRect(screen_item_min, screen_item_max, frame_color);

            // Strip extension of name, provides not benefit.
            std::string label = file->get_name();
            if (size_t extension_pos = label.find_last_of('.'); extension_pos != std::string::npos)
            {
                label = label.substr(0, extension_pos);
            }            

            ImVec2 label_size = ImGui::CalcTextSize(label.c_str(), nullptr, false, k_item_width - (k_item_padding * 2));
            ImVec2 text_pos = ImVec2(item_min.x + (item_bb.GetWidth() * 0.5f) - (label_size.x * 0.5f), item_min.y + item_bb.GetHeight() - label_size.y - k_item_padding);

            ImGui::SetCursorPos(text_pos);
            ImGui::PushTextWrapPos(item_bb.Min.x + item_bb.GetWidth() - k_item_padding);
            ImGui::Text("%s", label.c_str());
            ImGui::PopTextWrapPos();

            if (is_dragging)
            {
                ImGui::EndDragDropSource();
            }

            ImGui::PopID();
        }
    }
}

void editor_assets_window::import_asset()
{
    std::vector<asset_importer*> loaders = m_asset_manager->get_asset_importers();

    // Generate a filter list from all 
    std::vector<file_dialog_filter> filters;

    for (asset_importer* loader : loaders)
    {
        std::vector<std::string> extensions = loader->get_supported_extensions();

        file_dialog_filter& filter = filters.emplace_back();
        filter.name = loader->get_file_type_description();

        for (const std::string& ext : extensions)
        {
            if (ext[0] == '.')
            {
                filter.extensions.push_back(ext.substr(1));
            }
            else
            {
                filter.extensions.push_back(ext);
            }
        }
    }

    if (std::string path = open_file_dialog("Import File", filters); !path.empty())
    {
        std::string extension = virtual_file_system::get_extension(path.c_str());

        if (asset_importer* importer = m_asset_manager->get_importer_for_extension(extension.c_str()))
        {
            std::string output_path = m_selected_path;
            if (output_path.empty())
            {
                output_path = "data:";
            }

            db_log(engine, "Importing '%s' to '%s'.", path, output_path.c_str());
            if (!importer->import(path.c_str(), output_path.c_str()))
            {
                message_dialog("Failed to import asset, view output log for details.", message_dialog_type::error);
            }
        }
        else
        {
            message_dialog("Failed to find importer that supports this asset extension.", message_dialog_type::error);
        }
    }
}

void editor_assets_window::draw()
{ 
    if (m_open)
    {
        if (ImGui::Begin(get_window_id(), &m_open))
        {
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::Button("Import"))
            {
                import_asset();
            }

            ImGui::SameLine();
            ImGui::Text("Filter");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200.0f);
            ImGui::PushID("Filter");
            ImGui::InputText("", m_current_filter, sizeof(m_current_filter));
            ImGui::PopID();

            ImGui::SameLine();
            ImGui::Text("Type");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200.0f);
            ImGui::PushID("AssetType");
            ImGui::Combo("", &m_current_filter_type, m_filter_types, k_filter_type_count);
            ImGui::PopID();

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));

            ImGui::BeginChild("AssetView");

                // This is a ridiculously elaborate way of having a splitter.
                ImGuiID dockspace_id = ImGui::GetID("AssetDockspace");
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoDocking);

                if (m_first_frame)
                {
                    m_first_frame = false;

                    ImGui::DockBuilderRemoveNode(dockspace_id);
                    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoDocking);

                    ImGuiID dock_id_left, dock_id_right;
                    ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.25f, &dock_id_left, &dock_id_right);
                 
                    ImGui::DockBuilderDockWindow("AssetTree", dock_id_left);
                    ImGui::DockBuilderDockWindow("AssetList", dock_id_right);

                    ImGui::DockBuilderFinish(dockspace_id);
                }

                if (ImGui::Begin("AssetTree"))
                {
                    draw_asset_tree();
                }
                ImGui::End();

                if (ImGui::Begin("AssetList"))
                {
                    draw_asset_list();
                }
                ImGui::End();

            ImGui::EndChild();

            ImGui::PopStyleVar(1);
        }
        ImGui::End();
    }
}

const char* editor_assets_window::get_window_id()
{
    return "Assets";
}

editor_window_layout editor_assets_window::get_layout()
{
    return editor_window_layout::bottom_right;
}

}; // namespace ws
