// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.editor/editor/editor_window.h"
#include "workshop.core/debug/log_handler.h"
#include "workshop.editor/editor/utils/property_list.h"
#include "workshop.engine/ecs/object_manager.h"

namespace ws {

class world;
class editor;

// ================================================================================================
//  Window that shows the current objects properties
// ================================================================================================
class editor_properties_window 
    : public editor_window
{
public:
    editor_properties_window(editor* in_editor, engine* in_engine);

    virtual void draw() override;
    virtual const char* get_window_id() override;
    virtual editor_window_layout get_layout() override;

private:    
    void draw_add_component(object context);

private:
    engine* m_engine;
    editor* m_editor;

    object m_property_list_object;
    component* m_property_list_component;

    std::vector<uint8_t> m_before_modification_component;
    object m_pending_modifications_object;
    component* m_pending_modifications_component;
    bool m_pending_modifications = false;

    std::unique_ptr<property_list> m_property_list;

};

}; // namespace ws
