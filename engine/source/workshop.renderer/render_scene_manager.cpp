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
    , m_object_oct_tree(k_octtree_extents, k_octtree_max_depth)
{
    for (size_t i = 0; i < k_max_render_views; i++)
    {
        m_free_view_visibility_indices.push_back(i);
    }
}

void render_scene_manager::register_init(init_list& list)
{
}

void render_scene_manager::draw_cell_bounds(bool draw_cell_bounds, bool draw_object_bounds)
{
    auto cells = m_object_oct_tree.get_cells();
    for (auto& cell : cells)
    {
        if (draw_cell_bounds)
        {
            m_renderer.get_command_queue().draw_aabb(cell->bounds, color::green);
        }

        for (auto& entry : cell->elements)
        {
            if (draw_object_bounds)
            {
                m_renderer.get_command_queue().draw_aabb(entry.bounds, color::blue);
            }
        }
    }
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
        object->set_local_transform(location, rotation, scale);
    }
    else
    {
        db_warning(renderer, "set_object_transform called with non-existant id {%zi}.", id);
    }
}

void render_scene_manager::create_view(render_object_id id, const char* name)
{
    std::unique_ptr<render_view> view = std::make_unique<render_view>(this, m_renderer);
    view->set_name(name);

    render_view* view_ptr = view.get();

    auto [iter, success] = m_objects.try_emplace(id, std::move(view));
    if (success)
    {
        m_active_views.push_back(view_ptr);

        if (m_free_view_visibility_indices.empty())
        {
            db_error(renderer, "Ran out of visibility indices for new render view, consider reducing the number of active views or increase k_max_render_views: {%zi} %s", id, name);
            view_ptr->visibility_index = render_view::k_always_visible_index;
        }
        else
        {
            view_ptr->visibility_index = m_free_view_visibility_indices.back();
            m_free_view_visibility_indices.pop_back();
        }

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
        db_verbose(renderer, "Removed render view: {%zi} %s", id, iter->second->get_name().c_str());

        size_t visibility_index = static_cast<render_view*>(iter->second.get())->visibility_index;
        if (visibility_index != render_view::k_always_visible_index)
        {
            m_free_view_visibility_indices.push_back(visibility_index);
        }

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
        object->set_viewport(viewport);
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
        object->set_fov(fov);
        object->set_aspect_ratio(aspect_ratio);
        object->set_clip(near_clip, far_clip);
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
    std::unique_ptr<render_static_mesh> obj = std::make_unique<render_static_mesh>(this, m_renderer);
    obj->set_name(name);

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
        db_verbose(renderer, "Removed static mesh: {%zi} %s", id, iter->second->get_name().c_str());
        
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
        object->set_model(model);
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

void render_scene_manager::render_object_created(render_object* obj)
{
    obj->object_tree_token = m_object_oct_tree.insert(obj->get_bounds().get_aligned_bounds(), obj);
}

void render_scene_manager::render_object_destroyed(render_object* obj)
{
    if (obj->object_tree_token.is_valid())
    {
        m_object_oct_tree.remove(obj->object_tree_token);
        obj->object_tree_token.reset();
    }
}

void render_scene_manager::render_object_bounds_modified(render_object* obj)
{
    // Remove and readd to octtree with the new bounds.
    obj->object_tree_token = m_object_oct_tree.modify(obj->object_tree_token, obj->get_bounds().get_aligned_bounds(), obj);
}

void render_scene_manager::update_visibility()
{
    size_t frame_index = m_renderer.get_visibility_frame_index();

    for (render_view* view : m_active_views)
    {
        if (view->visibility_index == render_view::k_always_visible_index)
        {
            continue;
        }

        frustum frustum = view->get_frustum();

        decltype(m_object_oct_tree)::intersect_result visible_objects = m_object_oct_tree.intersect(frustum, false);
        for (render_object* object : visible_objects.elements)
        {
            object->last_visible_frame[view->visibility_index] = frame_index;
        }
    }
}

}; // namespace ws
