// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.editor/editor/editor_window.h"
#include "workshop.core/debug/log_handler.h"

namespace ws {

// ================================================================================================
//  Window that shows the memory usage
// ================================================================================================
class editor_memory_window 
    : public editor_window
{
public:
    editor_memory_window();

    virtual void draw() override;
    virtual const char* get_window_id() override;
    virtual editor_window_layout get_layout() override;

private:
    struct node
    {
        std::string name;
        std::string path;

        bool is_asset = false;

        size_t used;
        size_t peak;

        std::vector<node> children;
    };

    void add_node(node& parent, const char* path, const std::vector<std::string>& fragments, bool is_asset, size_t used_bytes, size_t allocation_count);

    void build_tree();
    void draw_tree();

private:
    node m_root;

};

}; // namespace ws
