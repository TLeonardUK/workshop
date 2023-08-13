// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.editor/editor/editor_window.h"

namespace ws {

// ================================================================================================
//  Window that shows the logging output of the game.
// ================================================================================================
class editor_log_window 
    : public editor_window
{
public:

    virtual void draw() override;
    virtual const char* get_window_id() override;
    virtual editor_window_layout get_layout() override;

};

}; // namespace ws
