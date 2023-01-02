// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_scene_manager.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/objects/render_view.h"

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
        db_warning(renderer, "set_object_transform called with non-existant view id {%zi}.", id);
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
        db_warning(renderer, "create_view called with a duplicate view id {%zi}.", id);
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
        db_warning(renderer, "destroy_view called with non-existant view id {%zi}.", id);
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
        db_warning(renderer, "set_view_viewport called with non-existant view id {%zi}.", id);
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
        db_warning(renderer, "set_view_projection called with non-existant view id {%zi}.", id);
    }
}

std::vector<render_view*> render_scene_manager::get_views()
{
    return m_active_views;
}

}; // namespace ws
