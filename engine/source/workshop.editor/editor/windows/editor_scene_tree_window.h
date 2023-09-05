// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.editor/editor/editor_window.h"
#include "workshop.core/debug/log_handler.h"
#include "workshop.engine/ecs/object.h"

namespace ws {

class engine;
class editor;
class transform_component;

// ================================================================================================
//  Window that shows the scenes current heirarchy.
// ================================================================================================
class editor_scene_tree_window
    : public editor_window
{
public:
    editor_scene_tree_window(editor* in_editor, engine* in_engine);

    virtual void draw() override;
    virtual const char* get_window_id() override;
    virtual editor_window_layout get_layout() override;

protected:
    void draw_object_node(object obj, transform_component* transform, size_t depth);

    void add_new_object();

private:
    engine* m_engine;
    editor* m_editor;
    
    std::vector<object> m_expanded_objects;

    bool m_clicked_item = false;
    object m_pending_delete = null_object;

};

}; // namespace ws
