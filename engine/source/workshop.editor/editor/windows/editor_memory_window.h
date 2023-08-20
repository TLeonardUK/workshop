// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.editor/editor/editor_window.h"
#include "workshop.editor/editor/utils/allocation_tree.h"
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
    void build_tree();


    void draw_node(const allocation_tree::node& node, size_t depth, const allocation_tree::node& root_node);
    void draw_tree();

private:
    allocation_tree m_allocation_tree;

    bool m_flat_view = false;
    char m_filter_buffer[256] = { '\0' };


};

}; // namespace ws
