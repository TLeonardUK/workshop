// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.editor/editor/editor_window.h"
#include "workshop.core/debug/log_handler.h"

namespace ws {

class editor_log_window;
class asset_manager;

// ================================================================================================
//  Window that shows the games asset loading state.
// ================================================================================================
class editor_loading_window 
    : public editor_window
{
public:
    editor_loading_window(asset_manager* ass_manager);

    virtual void draw() override;
    virtual const char* get_window_id() override;
    virtual editor_window_layout get_layout() override;

private:
    int m_load_state = 0;
    asset_manager* m_asset_manager = nullptr;

};

}; // namespace ws
