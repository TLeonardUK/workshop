// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_command_queue.h"
#include "workshop.renderer/renderer.h"

namespace ws {
    
render_command_queue::render_command_queue(renderer& render, size_t capacity)
    : command_queue(capacity)
    , m_renderer(render)
{
}

// ===========================================================================================
//  Global
// ===========================================================================================

void render_command_queue::set_visualization_mode(visualization_mode mode)
{
    queue_command("set_visualization_mode", [renderer = &m_renderer, mode]() {
        renderer->set_visualization_mode(mode);
    });
}

// ===========================================================================================
//  Objects
// ===========================================================================================

void render_command_queue::set_object_transform(render_object_id id, const vector3& location, const quat& rotation, const vector3& scale)
{
    queue_command("set_object_transform", [renderer = &m_renderer, id, location, rotation, scale]() {
        renderer->get_scene_manager().set_object_transform(id, location, rotation, scale);
    });
}

// ================================================================================================
//  Views
// ================================================================================================

render_object_id render_command_queue::create_view(const char* name)
{
    render_object_id id = m_renderer.next_render_object_id();
    const char* stored_name = allocate_copy(name);

    queue_command("create_view", [renderer = &m_renderer, id, stored_name]() {
        renderer->get_scene_manager().create_view(id, stored_name);
    });

    return id;
}

void render_command_queue::destroy_view(render_object_id id)
{
    queue_command("destroy_view", [renderer = &m_renderer, id]() {
        renderer->get_scene_manager().destroy_view(id);
    });
}

void render_command_queue::set_view_viewport(render_object_id id, const recti& viewport)
{
    queue_command("set_viewport", [renderer = &m_renderer, id, viewport]() {
        renderer->get_scene_manager().set_view_viewport(id, viewport);
    });
}

void render_command_queue::set_view_projection(render_object_id id, float fov, float aspect_ratio, float near_clip, float far_clip)
{
    queue_command("set_view_projection", [renderer = &m_renderer, id, fov, aspect_ratio, near_clip, far_clip]() {
        renderer->get_scene_manager().set_view_projection(id, fov, aspect_ratio, near_clip, far_clip);
    });
}

// ================================================================================================
//  Static meshes
// ================================================================================================

render_object_id render_command_queue::create_static_mesh(const char* name)
{
    render_object_id id = m_renderer.next_render_object_id();
    const char* stored_name = allocate_copy(name);

    queue_command("create_static_mesh", [renderer = &m_renderer, id, stored_name]() {
        renderer->get_scene_manager().create_static_mesh(id, stored_name);
    });

    return id;
}

void render_command_queue::destroy_static_mesh(render_object_id id)
{
    queue_command("destroy_static_mesh", [renderer = &m_renderer, id]() {
        renderer->get_scene_manager().destroy_static_mesh(id);
    });
}

void render_command_queue::set_static_mesh_model(render_object_id id, const asset_ptr<model>& model)
{
    queue_command("set_static_mesh_model", [renderer = &m_renderer, id, model]() {
        renderer->get_scene_manager().set_static_mesh_model(id, model);
    });
}

}; // namespace ws
