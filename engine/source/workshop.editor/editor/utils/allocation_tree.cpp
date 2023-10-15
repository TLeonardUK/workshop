// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/utils/allocation_tree.h"
#include "workshop.core/containers/string.h"

namespace ws {

allocation_tree::allocation_tree()
{
    clear();
}

bool allocation_tree::visit(node& parent, std::function<bool(node& visitee)> callback)
{
    for (auto iter = parent.children.begin(); iter != parent.children.end(); /*empty*/)
    {
        if (visit(*(*iter), callback))
        {
            iter = parent.children.erase(iter);
        }
        else
        {
            iter++;
        }
    }

    return callback(parent);
}

void allocation_tree::begin_mutate()
{
    visit(m_root, [](node& node) {
        node.old_allocation_count = node.allocation_count;
        node.old_peak_allocation_count = node.peak_allocation_count;
        node.old_used_size = node.used_size;
        node.old_peak_size = node.peak_size;

        node.allocation_count = 0;
        node.peak_allocation_count = 0;
        node.used_size = 0;
        node.peak_size = 0;

        node.is_used = false;
        node.unfiltered_children = 0;

        return false;
    });
}

void allocation_tree::end_mutate()
{
    // Remove any unused nodes.
    visit(m_root, [this](node& node) {

        // Mark dirty if size/allocation/etc has changed.
        if (node.old_allocation_count != node.allocation_count ||
            node.old_peak_allocation_count != node.peak_allocation_count ||
            node.old_used_size != node.used_size ||
            node.old_peak_size != node.peak_size)
        {
            node.is_dirty = true;
        }

        // Update filter/sorting if dirty.
        if (node.is_dirty || m_filter_changed)
        {
            node.is_filtered_out = (
                !m_filter.empty() && 
                node.filter_key.find(m_filter.c_str()) == std::string::npos && 
                node.filter_key.find(m_filter.c_str()) == std::string::npos &&
                node.filter_key.find(m_filter.c_str()) == std::string::npos
            );
            if (node.parent)
            {
                node.parent->is_pending_sort = true;
            }
            node.is_dirty = false;
        }

        // Count unfiltered children.
        if (!node.is_filtered_out)
        {
            node.unfiltered_children++;
        }
        for (auto& child : node.children)
        {
            node.unfiltered_children += child->unfiltered_children;
        }

        // Perform sort if pending.
        if (node.is_pending_sort)
        {
            std::sort(node.children.begin(), node.children.end(), [](auto& a, auto& b) {
                return b->used_size < a->used_size;
            });
            node.is_pending_sort = false;
        }

        // Remove if no longer used.
        return !node.is_used;

    });

    m_filter_changed = false;
}

void allocation_tree::add(const std::string& display_path, const std::string& meta_path, size_t size, size_t allocation_count)
{
    std::vector<std::string> fragments = string_split(display_path, "/");
    add_node(m_root, display_path.c_str(), meta_path.c_str(), fragments, size, allocation_count);

    if (meta_path[0] == '\0')
    {
        m_root.used_size += size;
        m_root.peak_size = std::max(m_root.peak_size, m_root.used_size);
        m_root.allocation_count += allocation_count;
        m_root.peak_allocation_count = std::max(m_root.peak_allocation_count, m_root.allocation_count);
    }
}

void allocation_tree::filter(const char* value)
{
    m_filter = string_lower(value);
    m_filter_changed = true;
}

void allocation_tree::clear()
{
    m_root = {};
    m_root.name = "/";
    m_root.display_path = "/";
}

const allocation_tree::node& allocation_tree::get_root()
{
    return m_root;
}

void allocation_tree::add_node(node& parent, const char* display_path, const char* meta_path, const std::vector<std::string>& fragments, size_t used_bytes, size_t allocation_count)
{
    const std::string& current_fragment = fragments.front();
    auto iter = std::find_if(parent.children.begin(), parent.children.end(), [current_fragment](std::unique_ptr<node>& child) {
        return child->name == current_fragment;
    });

    if (iter == parent.children.end())
    {
        std::unique_ptr<node> new_node = std::make_unique<node>();
        node& child = *new_node;
        parent.children.push_back(std::move(new_node));

        child.parent = &parent;
        child.name = current_fragment;
        child.display_path = display_path;
        child.meta_path = meta_path;
        child.is_used = true;
        child.is_dirty = true;

        child.filter_key = string_lower(child.name + " " + child.display_path + " " + child.meta_path);

        iter = parent.children.begin() + (parent.children.size() - 1);
    }
    else
    {
        (*iter)->is_used = true;
    }

    node& child_node = *(*iter);

    if (fragments.size() == 1)
    {
        child_node.exclusive_size += used_bytes;
        child_node.exclusive_peak_size = std::max(child_node.exclusive_peak_size, child_node.exclusive_size);

        child_node.used_size += used_bytes;
        child_node.peak_size = std::max(child_node.peak_size, used_bytes);
        child_node.allocation_count += allocation_count;
        child_node.peak_allocation_count = std::max(child_node.peak_allocation_count, allocation_count);
    }
    else
    {
        std::vector<std::string> remaining_fragments = fragments;
        remaining_fragments.erase(remaining_fragments.begin());

        if (meta_path[0] == '\0')
        {
            if (used_bytes > 0 || allocation_count > 0)
            {
                child_node.used_size += used_bytes;
                child_node.peak_size = std::max(child_node.peak_size, child_node.used_size);
                child_node.allocation_count += allocation_count;
                child_node.peak_allocation_count = std::max(child_node.peak_allocation_count, child_node.allocation_count);
            }
        }

        add_node(child_node, display_path, meta_path, remaining_fragments, used_bytes, allocation_count);
    }
}

}; // namespace ws
