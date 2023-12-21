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
//  Window that shows the current state of all cvars
// ================================================================================================
class editor_cvar_window 
    : public editor_window
{
public:
    editor_cvar_window();

    virtual void draw() override;
    virtual const char* get_window_id() override;
    virtual editor_window_layout get_layout() override;

private:
    char m_filter_buffer[256] = { '\0' };

};

}; // namespace ws
