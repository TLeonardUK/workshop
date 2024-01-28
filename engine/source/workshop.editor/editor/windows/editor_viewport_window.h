// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.editor/editor/editor_window.h"
#include "workshop.core/debug/log_handler.h"
#include "workshop.engine/ecs/object.h"
#include "workshop.renderer/renderer.h"
#include "workshop.core/utils/event.h"

namespace ws {

class engine;
class editor;
class world;
class transform_component;

// ================================================================================================
//  Window that shows the an instance of the viewport.
// ================================================================================================
class editor_viewport_window
    : public editor_window
{
public:
    editor_viewport_window(editor* in_editor, engine* in_engine, size_t index);
    ~editor_viewport_window();

    virtual void draw() override;
    virtual const char* get_window_id() override;
    virtual editor_window_layout get_layout() override;

private:
    void recreate_views();
    void update_render_target(bool initial_update);

private:
    engine* m_engine;
    editor* m_editor;

    size_t m_viewport_index;

    object m_view_camera;

    event<world*>::delegate_ptr m_on_default_world_changed_delegate;

    std::unique_ptr<ri_texture> m_render_target;
    std::vector<std::unique_ptr<ri_texture>> m_render_target_remove_queue;
    ri_texture* m_current_render_target = nullptr;

    char m_name[128];
    
};

}; // namespace ws
