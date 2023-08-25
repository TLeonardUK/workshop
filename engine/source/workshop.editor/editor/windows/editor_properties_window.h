// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.editor/editor/editor_window.h"
#include "workshop.core/debug/log_handler.h"

namespace ws {

// ================================================================================================
//  Window that shows the current objects properties
// ================================================================================================
class editor_properties_window 
    : public editor_window
{
public:
    editor_properties_window();

    virtual void draw() override;
    virtual const char* get_window_id() override;
    virtual editor_window_layout get_layout() override;

private:

};

}; // namespace ws
