// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.editor/editor/editor_window.h"

#include <string>

namespace ws {

// ================================================================================================
//  Window that pops ups to show the progress of saving/loading.
// ================================================================================================
class editor_progress_popup
    : public editor_window
{
public:
    editor_progress_popup();

    void set_title(const char* text);
    void set_subtitle(const char* text);
    void set_progress(float progress);

    virtual void draw() override;
    virtual const char* get_window_id() override;
    virtual editor_window_layout get_layout() override;

private:
    std::string m_title;
    std::string m_subtitle;
    float m_progress;

    bool m_was_open = false;

};

}; // namespace ws
