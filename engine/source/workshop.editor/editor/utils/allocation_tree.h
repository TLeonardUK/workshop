// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/debug/log_handler.h"

#include <functional>

namespace ws {

// ================================================================================================
//  Represents a tree of allocations that can be sorted/filtered/etc
// ================================================================================================
class allocation_tree 
{
public:
    struct node
    {
        node* parent = nullptr;

        std::string name;
        std::string display_path;
        std::string meta_path;
        std::string filter_key;

        size_t used_size = 0;
        size_t peak_size = 0;
        size_t allocation_count = 0;
        size_t peak_allocation_count = 0;

        size_t exclusive_size = 0;
        size_t exclusive_peak_size = 0;

        size_t old_used_size = 0;
        size_t old_peak_size = 0;
        size_t old_allocation_count = 0;
        size_t old_peak_allocation_count = 0;

        std::vector<std::unique_ptr<node>> children;

        size_t unfiltered_children = 0;

        bool is_used = false;      
        bool is_filtered_out = false;
        bool is_dirty = false;
        bool is_pending_sort = false;
    };

public:

    allocation_tree();

    // Must be called before doing any modifications are done to the tree.
    void begin_mutate();

    // Must be called after doing any modifications are done to the tree.
    void end_mutate();

    // Adds an callocation to the tree.
    void add(const std::string& display_path, const std::string& meta_path, size_t size, size_t allocation_count);

    // Filters the tree so only node without the given value are marked as is_filtered_out.
    void filter(const char* value);

    // Clears all entries in the tree.
    void clear();

    // Gets the root of the tree.
    const node& get_root();

private:
    void add_node(node& parent, const char* display_path, const char* meta_path, const std::vector<std::string>& fragments, size_t used_bytes, size_t allocation_count);

    bool visit(node& parent, std::function<bool(node& visitee)> callback);

private:
    node m_root;

    std::string m_filter;
    bool m_filter_changed = false;

};

}; // namespace ws
