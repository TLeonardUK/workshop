// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/windows/editor_memory_window.h"
#include "workshop.core/containers/string.h"
#include "workshop.core/memory/memory_tracker.h"
#include "workshop.core/utils/string_formatter.h"

#include "thirdparty/imgui/imgui.h"

namespace ws {

editor_memory_window::editor_memory_window()
{
}

void editor_memory_window::build_tree()
{
    memory_tracker& tracker = memory_tracker::get();

    m_allocation_tree.begin_mutate();

    for (size_t i = 0; i < static_cast<size_t>(memory_type::COUNT); i++)
    {
        memory_type type = static_cast<memory_type>(i);
        std::string path = memory_type_names[i];

        size_t used_bytes = tracker.get_memory_used_bytes(type);
        size_t alloc_count = tracker.get_memory_allocation_count(type);

        if (used_bytes == 0)
        {
            continue;
        }

        // Add this memory tag to tree.
        m_allocation_tree.add(path, "", used_bytes, alloc_count);

        // Get breakdown of assets in this tag and add them.
        std::vector<memory_tracker::asset_state> assets = tracker.get_assets(type);
        for (auto& state : assets)
        {
            std::string base_asset_path = state.id.get_string();
            std::string asset_path = base_asset_path;

            // Only append the assets filename to the tree path.
            auto iter = asset_path.find_last_of('/');
            if (iter != std::string::npos)
            {
                asset_path = asset_path.substr(iter + 1);
            }
            asset_path = path + "/" + asset_path;

            m_allocation_tree.add(asset_path, base_asset_path, state.used_bytes, state.allocation_count);
        }
    }

    m_allocation_tree.end_mutate();
}

void editor_memory_window::draw_node(const allocation_tree::node& node, size_t depth, const allocation_tree::node& root_node)
{
    float indent = (depth * 10.0f) + 0.01f;

    ImGui::TableNextRow();

    // Name
    ImGui::TableNextColumn(); 

    size_t used_bytes = node.used_size;
    size_t peak_bytes = node.peak_size;
    size_t alloc_count = node.allocation_count;

    bool draw_children = false;
    if (m_flat_view)
    {
        ImGui::Text("%s", node.name.c_str());
        draw_children = true;

        used_bytes = node.exclusive_size;
        peak_bytes = node.exclusive_peak_size;
    }
    else
    {
        ImGui::Indent(indent);

        if (node.unfiltered_children == 0 || node.children.empty())
        {
            ImGui::Text("%s", node.name.c_str());
        }
        else
        {
            draw_children = ImGui::CollapsingHeader(node.name.c_str(), ImGuiTreeNodeFlags_None);        
        }

        ImGui::Unindent(indent);
    }

    // Bytes Used
    ImGui::TableNextColumn(); 

    string_formatter used_overlay;
    used_overlay.format("%.1f MB", static_cast<float>(used_bytes) / (1024.0f * 1024.0f));

    ImGui::ProgressBar(static_cast<float>(used_bytes) / static_cast<float>(root_node.used_size), ImVec2(-FLT_MIN, 0), used_overlay.c_str());

    // Peak Memory Used
    ImGui::TableNextColumn(); ImGui::Text("%.1f MB", static_cast<float>(peak_bytes) / (1024.0f * 1024.0f));

    // Allocation Count
    ImGui::TableNextColumn(); ImGui::Text("%zi", alloc_count);

    // Meta path
    ImGui::TableNextColumn(); ImGui::Text("%s", node.meta_path.c_str());

    // Draw children.
    if (draw_children)
    {
        for (auto& child : node.children)
        {
            if (child->unfiltered_children > 0)
            {
                draw_node(*child, depth + 1, root_node);
            }
        }
    }
}

void editor_memory_window::draw_tree()
{
    draw_node(m_allocation_tree.get_root(), 0, m_allocation_tree.get_root());
}

void editor_memory_window::draw()
{
    if (m_open)
    {
        if (ImGui::Begin(get_window_id(), &m_open))
        {
            if (ImGui::Button(m_flat_view ? "Tree View" : "Flat View"))
            {
                m_flat_view = !m_flat_view;
            }
            ImGui::SameLine();
            ImGui::Text("Filter");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::InputText("##", m_filter_buffer, sizeof(m_filter_buffer)))
            {
                m_allocation_tree.filter(m_filter_buffer);
            }

            ImGui::BeginChild("MemoryTableView");
            ImGui::BeginTable("MemoryTable", 5, ImGuiTableFlags_Resizable);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.35f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.1f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.1f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.1f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.35f);

            ImGui::TableNextColumn(); ImGui::TableHeader("Path");
            if (m_flat_view)
            {
                ImGui::TableNextColumn(); ImGui::TableHeader("Exclusive Memory");
                ImGui::TableNextColumn(); ImGui::TableHeader("Exclusive Peak Memory");
            }
            else
            {
                ImGui::TableNextColumn(); ImGui::TableHeader("Inclusive Memory");
                ImGui::TableNextColumn(); ImGui::TableHeader("Inclusive Peak Memory");
            }
            ImGui::TableNextColumn(); ImGui::TableHeader("Allocations");
            ImGui::TableNextColumn(); ImGui::TableHeader("Description");

            build_tree();
            draw_tree();

            ImGui::EndTable();
            ImGui::EndChild();
        }
        ImGui::End();
    }
}

const char* editor_memory_window::get_window_id()
{
    return "Memory";
}

editor_window_layout editor_memory_window::get_layout()
{
    return editor_window_layout::bottom;
}

}; // namespace ws
