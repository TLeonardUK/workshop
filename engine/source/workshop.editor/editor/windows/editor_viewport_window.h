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

#include "workshop.game_framework/systems/transform/object_pick_system.h"

#include <future>

struct ImRect;

namespace ws {

class engine;
class editor;
class world;
class transform_component;

enum class viewport_orientation
{
    none,
    perspective,
    ortho_x_neg,
    ortho_x_pos,
    ortho_y_neg,
    ortho_y_pos,
    ortho_z_neg,
    ortho_z_pos,
};

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

    void update_camera_perspective();

    void set_orientation(viewport_orientation new_orientation);

    void toggle_view_flag(render_view_flags flag);

    void update_object_picking(bool mouse_over_viewport, const ImRect& viewport_rect);

    void update_drag_drop(bool mouse_over_viewport, const ImRect& viewport_rect);

private:
    engine* m_engine;
    editor* m_editor;

    size_t m_viewport_index;
    vector2 m_viewport_size = vector2::zero;

    render_view_flags m_render_view_flags = 
        render_view_flags::normal | 
        render_view_flags::render_in_editor_mode |
        render_view_flags::lazy_render;

    object m_view_camera;

    event<world*>::delegate_ptr m_on_default_world_changed_delegate;

    std::unique_ptr<ri_texture> m_render_target;
    std::vector<std::unique_ptr<ri_texture>> m_render_target_remove_queue;
    ri_texture* m_current_render_target = nullptr;

    viewport_orientation m_orientation = viewport_orientation::none;

    char m_name[128];
    
    // Object selection
    std::future<object_pick_system::pick_result> m_pick_object_query;
    bool m_pick_object_add_to_selected = false;

    // Lazy update
    bool m_realtime = false;
    bool m_new_render_required = true;
    event<>::delegate_ptr m_transaction_executed_delegate;

    // Drag/Drop
    std::future<object_pick_system::pick_result> m_drag_drop_pick_query;
    object m_drag_drop_object = null_object;

};

}; // namespace ws
