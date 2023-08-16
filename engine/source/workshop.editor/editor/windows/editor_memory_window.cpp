// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/windows/editor_memory_window.h"
#include "workshop.core/containers/string.h"
#include "workshop.core/memory/memory_tracker.h"

#include "thirdparty/imgui/imgui.h"

namespace ws {

editor_memory_window::editor_memory_window()
{
}

void editor_memory_window::add_node(node& parent, const char* path, const std::vector<std::string>& fragments, bool is_asset, size_t used_bytes, size_t allocation_count)
{
    const std::string& current_fragment = fragments.back();
    auto iter = std::find_if(parent.children.begin(), parent.children.end(), [current_fragment](node& child) {
        return child.name == current_fragment;
    });

    if (iter == parent.children.end())
    {
        node& child = parent.children.emplace_back();
        child.name = current_fragment;
        child.path = path;

        iter = parent.children.begin() + (parent.children.size() - 1);
    }

    if (fragments.size() == 1)
    {
        iter->is_asset = is_asset;
        iter->used = used_bytes;
        iter->peak = std::max(iter->peak, iter->used);
    }
    else
    {
        std::vector<std::string> remaining_fragments = fragments;
        remaining_fragments.pop_back();

        if (!is_asset)
        {
            iter->used += used_bytes;
            iter->peak = std::max(iter->peak, iter->used);
        }

        add_node(*iter, path, remaining_fragments, is_asset, used_bytes, allocation_count);
    }
}

void editor_memory_window::build_tree()
{
    memory_tracker& tracker = memory_tracker::get();

    for (size_t i = 0; i < static_cast<size_t>(memory_type::COUNT); i++)
    {
        memory_type type = static_cast<memory_type>(i);
        std::string path = memory_type_names[i];

        // Add this memory tag to tree.
        std::vector<std::string> fragments = string_split(path, "/");
        add_node(m_root, path.c_str(), fragments, false, tracker.get_memory_used_bytes(type), tracker.get_memory_allocation_count(type));

        // Get breakdown of assets in this tag and add them.
        std::vector<memory_tracker::asset_state> assets = tracker.get_assets(type);
        for (auto& state : assets)
        {
            std::string asset_path = state.id.get_string();
            asset_path = path + "/" + asset_path;

            std::vector<std::string> fragments = string_split(asset_path, "/");
            add_node(m_root, asset_path.c_str(), fragments, true, state.used_bytes, state.allocation_count);
        }
    }
}

void editor_memory_window::draw_tree()
{
}

void editor_memory_window::draw()
{
    if (m_open)
    {
        if (ImGui::Begin(get_window_id(), &m_open))
        {
            ImGui::Text("Memory Profiling");

            build_tree();

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
