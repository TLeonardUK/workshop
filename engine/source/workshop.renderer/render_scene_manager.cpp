// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_scene_manager.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.renderer/objects/render_static_mesh.h"

namespace ws {
    
render_scene_manager::render_scene_manager(renderer& render)
    : m_renderer(render)
{
}

void render_scene_manager::register_init(init_list& list)
{
}

render_object* render_scene_manager::resolve_id(render_object_id id)
{
    if (auto iter = m_objects.find(id); iter != m_objects.end())
    {
        return iter->second.get();
    }
    return nullptr;
}

void render_scene_manager::set_object_transform(render_object_id id, const vector3& location, const quat& rotation, const vector3& scale)
{
    if (render_object* object = resolve_id(id))
    {
        object->local_location = location;
        object->local_rotation = rotation;
        object->local_scale = scale;
    }
    else
    {
        db_warning(renderer, "set_object_transform called with non-existant id {%zi}.", id);
    }
}

void render_scene_manager::create_view(render_object_id id, const char* name)
{
    std::unique_ptr<render_view> view = std::make_unique<render_view>();
    view->name = name;

    render_view* view_ptr = view.get();

    auto [iter, success] = m_objects.try_emplace(id, std::move(view));
    if (success)
    {
        m_active_views.push_back(view_ptr);

        db_verbose(renderer, "Created new render view: {%zi} %s", id, name);
    }
    else
    {
        db_warning(renderer, "create_view called with a duplicate id {%zi}.", id);
    }
}

void render_scene_manager::destroy_view(render_object_id id)
{
    if (auto iter = m_objects.find(id); iter != m_objects.end())
    {
        db_verbose(renderer, "Removed render view: {%zi} %s", id, iter->second->name.c_str());

        m_active_views.erase(std::find(m_active_views.begin(), m_active_views.end(), iter->second.get()));
        m_objects.erase(iter);
    }
    else
    {
        db_warning(renderer, "destroy_view called with non-existant id {%zi}.", id);
    }
}

void render_scene_manager::set_view_viewport(render_object_id id, const recti& viewport)
{
    if (render_view* object = dynamic_cast<render_view*>(resolve_id(id)))
    {
        object->viewport = viewport;
    }
    else
    {
        db_warning(renderer, "set_view_viewport called with non-existant id {%zi}.", id);
    }
}

void render_scene_manager::set_view_projection(render_object_id id, float fov, float aspect_ratio, float near_clip, float far_clip)
{
    if (render_view* object = dynamic_cast<render_view*>(resolve_id(id)))
    {
        object->field_of_view = fov;
        object->aspect_ratio = aspect_ratio;
        object->near_clip = near_clip;
        object->far_clip = far_clip;
    }
    else
    {
        db_warning(renderer, "set_view_projection called with non-existant id {%zi}.", id);
    }
}

std::vector<render_view*> render_scene_manager::get_views()
{
    return m_active_views;
}

void render_scene_manager::create_static_mesh(render_object_id id, const char* name)
{
    std::unique_ptr<render_static_mesh> obj = std::make_unique<render_static_mesh>();
    obj->name = name;

    render_static_mesh* obj_ptr = obj.get();

    auto [iter, success] = m_objects.try_emplace(id, std::move(obj));
    if (success)
    {
        m_active_static_meshes.push_back(obj_ptr);

        db_verbose(renderer, "Created new static mesh: {%zi} %s", id, name);
    }
    else
    {
        db_warning(renderer, "create_static_mesh called with a duplicate id {%zi}.", id);
    }
}

void render_scene_manager::destroy_static_mesh(render_object_id id)
{
    if (auto iter = m_objects.find(id); iter != m_objects.end())
    {
        db_verbose(renderer, "Removed static mesh: {%zi} %s", id, iter->second->name.c_str());

        m_active_static_meshes.erase(std::find(m_active_static_meshes.begin(), m_active_static_meshes.end(), iter->second.get()));
        m_objects.erase(iter);
    }
    else
    {
        db_warning(renderer, "destroy_static_mesh called with non-existant id {%zi}.", id);
    }
}

void render_scene_manager::set_static_mesh_model(render_object_id id, const asset_ptr<model>& model)
{
    if (render_static_mesh* object = dynamic_cast<render_static_mesh*>(resolve_id(id)))
    {
        object->model = model;
    }
    else
    {
        db_warning(renderer, "set_static_mesh_model called with non-existant id {%zi}.", id);
    }
}

std::vector<render_static_mesh*> render_scene_manager::get_static_meshes()
{
    return m_active_static_meshes;
}

}; // namespace ws
