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
    void set_context(object obj);

    void draw_add_component();

private:
    engine* m_engine;
    editor* m_editor;

    struct component_info
    {
        std::unique_ptr<property_list> property_list;
        component* component;
        std::string name;
        event<reflect_field*>::delegate_ptr on_modified_delegate;
    };

    object m_context;
    std::vector<component_info> m_component_info;

};

}; // namespace ws
