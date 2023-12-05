// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.editor/editor/editor_window.h"
#include "workshop.core/debug/log_handler.h"

namespace ws {

class renderer;

// ================================================================================================
//  Window that shows the current state of the renderer texture streamer.
// ================================================================================================
class editor_texture_streaming_window 
    : public editor_window
{
public:
    editor_texture_streaming_window(renderer* render);

    virtual void draw() override;
    virtual const char* get_window_id() override;
    virtual editor_window_layout get_layout() override;

private:
    renderer* m_renderer = nullptr;

    int m_state_filter = 0;

    char m_filter_buffer[256] = { '\0' };

};

}; // namespace ws
