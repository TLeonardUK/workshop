// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_scene_manager.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.renderer/objects/render_static_mesh.h"
#include "workshop.renderer/objects/render_directional_light.h"
#include "workshop.renderer/objects/render_point_light.h"
#include "workshop.renderer/objects/render_spot_light.h"
#include "workshop.renderer/objects/render_light_probe_grid.h"

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
    std::scoped_lock lock(m_mutex);

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

void render_scene_manager::render_object_created(render_object* obj)
{
    std::scoped_lock lock(m_mutex);

    obj->object_tree_token = m_object_oct_tree.insert(obj->get_bounds().get_aligned_bounds(), obj);
}

void render_scene_manager::render_object_destroyed(render_object* obj)
{
    std::scoped_lock lock(m_mutex);

    if (obj->object_tree_token.is_valid())
    {
        m_object_oct_tree.remove(obj->object_tree_token);
        obj->object_tree_token.reset();
    }
}

void render_scene_manager::render_object_bounds_modified(render_object* obj)
{
    std::scoped_lock lock(m_mutex);

    // Remove and readd to octtree with the new bounds.
    obj->object_tree_token = m_object_oct_tree.modify(obj->object_tree_token, obj->get_bounds().get_aligned_bounds(), obj);
}

void render_scene_manager::update_visibility()
{
    std::scoped_lock lock(m_mutex);

    profile_marker(profile_colors::render, "update visibility");

    size_t frame_index = m_renderer.get_visibility_frame_index();

    parallel_for("update views", task_queue::standard, m_active_views.size(), [this, frame_index](size_t index) {

        profile_marker(profile_colors::render, "update view visibility");

        render_view* view = m_active_views[index];

        if (view->visibility_index == render_view::k_always_visible_index)
        {
            return;
        }

        frustum frustum = view->get_frustum();

        decltype(m_object_oct_tree)::intersect_result visible_objects = m_object_oct_tree.intersect(frustum, false, false);
        for (render_object* object : visible_objects.elements)
        {
            object->last_visible_frame[view->visibility_index] = frame_index;
        }

        // Mark view as dirty if something in the view has changed.
        if (view->get_last_change() != visible_objects.last_changed)
        {
            view->mark_dirty(visible_objects.last_changed);
        }

    });
}


render_object* render_scene_manager::resolve_id(render_object_id id)
{
    std::scoped_lock lock(m_mutex);

    if (auto iter = m_objects.find(id); iter != m_objects.end())
    {
        return iter->second.get();
    }

    return nullptr;
}

// ===========================================================================================
//  Objects
// ===========================================================================================

void render_scene_manager::set_object_transform(render_object_id id, const vector3& location, const quat& rotation, const vector3& scale)
{
    std::scoped_lock lock(m_mutex);

    if (render_object* object = resolve_id(id))
    {
        object->set_local_transform(location, rotation, scale);
    }
    else
    {
        db_warning(renderer, "set_object_transform called with non-existant id {%zi}.", id);
    }
}

// ===========================================================================================
//  Views
// ===========================================================================================

void render_scene_manager::create_view(render_object_id id, const char* name)
{
    std::scoped_lock lock(m_mutex);

    std::unique_ptr<render_view> view = std::make_unique<render_view>(id, this, m_renderer);
    view->init();
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
    std::scoped_lock lock(m_mutex);

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
    std::scoped_lock lock(m_mutex);

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
    std::scoped_lock lock(m_mutex);

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

// ===========================================================================================
//  Static meshes
// ===========================================================================================

void render_scene_manager::create_static_mesh(render_object_id id, const char* name)
{
    std::scoped_lock lock(m_mutex);

    std::unique_ptr<render_static_mesh> obj = std::make_unique<render_static_mesh>(id, this, m_renderer);
    obj->init();
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
    std::scoped_lock lock(m_mutex);

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
    std::scoped_lock lock(m_mutex);

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
    std::scoped_lock lock(m_mutex);

    return m_active_static_meshes;
}

// ===========================================================================================
//  Lights
// ===========================================================================================

void render_scene_manager::set_light_intensity(render_object_id id, float value)
{
    std::scoped_lock lock(m_mutex);

    if (render_light* object = dynamic_cast<render_light*>(resolve_id(id)))
    {
        object->set_intensity(value);
    }
    else
    {
        db_warning(renderer, "set_light_intensity called with non-existant id {%zi}.", id);
    }
}

void render_scene_manager::set_light_range(render_object_id id, float value)
{
    std::scoped_lock lock(m_mutex);

    if (render_light* object = dynamic_cast<render_light*>(resolve_id(id)))
    {
        object->set_range(value);
    }
    else
    {
        db_warning(renderer, "set_light_range called with non-existant id {%zi}.", id);
    }
}

void render_scene_manager::set_light_importance_distance(render_object_id id, float value)
{
    std::scoped_lock lock(m_mutex);

    if (render_light* object = dynamic_cast<render_light*>(resolve_id(id)))
    {
        object->set_importance_distance(value);
    }
    else
    {
        db_warning(renderer, "set_light_importance_distance called with non-existant id {%zi}.", id);
    }
}

void render_scene_manager::set_light_color(render_object_id id, color value)
{
    std::scoped_lock lock(m_mutex);

    if (render_light* object = dynamic_cast<render_light*>(resolve_id(id)))
    {
        object->set_color(value);
    }
    else
    {
        db_warning(renderer, "set_light_color called with non-existant id {%zi}.", id);
    }
}

void render_scene_manager::set_light_shadow_casting(render_object_id id, bool value)
{
    std::scoped_lock lock(m_mutex);

    if (render_light* object = dynamic_cast<render_light*>(resolve_id(id)))
    {
        object->set_shadow_casting(value);
    }
    else
    {
        db_warning(renderer, "set_light_shadow_casting called with non-existant id {%zi}.", id);
    }
}

void render_scene_manager::set_light_shadow_map_size(render_object_id id, size_t value)
{
    std::scoped_lock lock(m_mutex);

    if (render_light* object = dynamic_cast<render_light*>(resolve_id(id)))
    {
        object->set_shadow_map_size(value);
    }
    else
    {
        db_warning(renderer, "set_light_shadow_map_size called with non-existant id {%zi}.", id);
    }
}

void render_scene_manager::set_light_shadow_max_distance(render_object_id id, float value)
{
    std::scoped_lock lock(m_mutex);

    if (render_light* object = dynamic_cast<render_light*>(resolve_id(id)))
    {
        object->set_shadow_max_distance(value);
    }
    else
    {
        db_warning(renderer, "set_light_shadow_max_distance called with non-existant id {%zi}.", id);
    }
}

// ===========================================================================================
//  Directional light.
// ===========================================================================================

void render_scene_manager::create_directional_light(render_object_id id, const char* name)
{
    std::scoped_lock lock(m_mutex);

    std::unique_ptr<render_directional_light> obj = std::make_unique<render_directional_light>(id, this, m_renderer);
    obj->init();
    obj->set_name(name);

    render_directional_light* obj_ptr = obj.get();

    auto [iter, success] = m_objects.try_emplace(id, std::move(obj));
    if (success)
    {
        m_active_directional_lights.push_back(obj_ptr);

        db_verbose(renderer, "Created new directional light: {%zi} %s", id, name);
    }
    else
    {
        db_warning(renderer, "create_directional_light called with a duplicate id {%zi}.", id);
    }
}

void render_scene_manager::destroy_directional_light(render_object_id id)
{
    std::scoped_lock lock(m_mutex);

    if (auto iter = m_objects.find(id); iter != m_objects.end())
    {
        db_verbose(renderer, "Removed directional light: {%zi} %s", id, iter->second->get_name().c_str());

        m_active_directional_lights.erase(std::find(m_active_directional_lights.begin(), m_active_directional_lights.end(), iter->second.get()));
        m_objects.erase(iter);
    }
    else
    {
        db_warning(renderer, "destroy_directional_light called with non-existant id {%zi}.", id);
    }
}

void render_scene_manager::set_directional_light_shadow_cascades(render_object_id id, size_t value)
{
    std::scoped_lock lock(m_mutex);

    if (render_directional_light* object = dynamic_cast<render_directional_light*>(resolve_id(id)))
    {
        object->set_shadow_cascades(value);
    }
    else
    {
        db_warning(renderer, "set_directional_light_shadow_cascades called with non-existant id {%zi}.", id);
    }
}

void render_scene_manager::set_directional_light_shadow_cascade_exponent(render_object_id id, float value)
{
    std::scoped_lock lock(m_mutex);

    if (render_directional_light* object = dynamic_cast<render_directional_light*>(resolve_id(id)))
    {
        object->set_shadow_cascade_exponent(value);
    }
    else
    {
        db_warning(renderer, "set_directional_light_shadow_cascade_exponent called with non-existant id {%zi}.", id);
    }
}

void render_scene_manager::set_directional_light_shadow_cascade_blend(render_object_id id, float value)
{
    std::scoped_lock lock(m_mutex);

    if (render_directional_light* object = dynamic_cast<render_directional_light*>(resolve_id(id)))
    {
        object->set_shadow_cascade_blend(value);
    }
    else
    {
        db_warning(renderer, "set_directional_light_shadow_cascade_exponent called with non-existant id {%zi}.", id);
    }
}

std::vector<render_directional_light*> render_scene_manager::get_directional_lights()
{
    std::scoped_lock lock(m_mutex);

    return m_active_directional_lights;
}

// ===========================================================================================
//  Point light
// ===========================================================================================

void render_scene_manager::create_point_light(render_object_id id, const char* name)
{
    std::scoped_lock lock(m_mutex);

    std::unique_ptr<render_point_light> obj = std::make_unique<render_point_light>(id, this, m_renderer);
    obj->init();
    obj->set_name(name);

    render_point_light* obj_ptr = obj.get();

    auto [iter, success] = m_objects.try_emplace(id, std::move(obj));
    if (success)
    {
        m_active_point_lights.push_back(obj_ptr);

        db_verbose(renderer, "Created new point light: {%zi} %s", id, name);
    }
    else
    {
        db_warning(renderer, "create_point_light called with a duplicate id {%zi}.", id);
    }
}

void render_scene_manager::destroy_point_light(render_object_id id)
{
    std::scoped_lock lock(m_mutex);

    if (auto iter = m_objects.find(id); iter != m_objects.end())
    {
        db_verbose(renderer, "Removed point light: {%zi} %s", id, iter->second->get_name().c_str());

        m_active_point_lights.erase(std::find(m_active_point_lights.begin(), m_active_point_lights.end(), iter->second.get()));
        m_objects.erase(iter);
    }
    else
    {
        db_warning(renderer, "destroy_point_light called with non-existant id {%zi}.", id);
    }
}

std::vector<render_point_light*> render_scene_manager::get_point_lights()
{
    std::scoped_lock lock(m_mutex);

    return m_active_point_lights;
}

// ===========================================================================================
//  Spot light
// ===========================================================================================

void render_scene_manager::create_spot_light(render_object_id id, const char* name)
{
    std::scoped_lock lock(m_mutex);

    std::unique_ptr<render_spot_light> obj = std::make_unique<render_spot_light>(id, this, m_renderer);
    obj->init();
    obj->set_name(name);

    render_spot_light* obj_ptr = obj.get();

    auto [iter, success] = m_objects.try_emplace(id, std::move(obj));
    if (success)
    {
        m_active_spot_lights.push_back(obj_ptr);

        db_verbose(renderer, "Created new spot light: {%zi} %s", id, name);
    }
    else
    {
        db_warning(renderer, "create_spot_light called with a duplicate id {%zi}.", id);
    }
}

void render_scene_manager::destroy_spot_light(render_object_id id)
{
    std::scoped_lock lock(m_mutex);

    if (auto iter = m_objects.find(id); iter != m_objects.end())
    {
        db_verbose(renderer, "Removed spot light: {%zi} %s", id, iter->second->get_name().c_str());

        m_active_spot_lights.erase(std::find(m_active_spot_lights.begin(), m_active_spot_lights.end(), iter->second.get()));
        m_objects.erase(iter);
    }
    else
    {
        db_warning(renderer, "destroy_point_light called with non-existant id {%zi}.", id);
    }
}

void render_scene_manager::set_spot_light_radius(render_object_id id, float inner_radius, float outer_radius)
{
    std::scoped_lock lock(m_mutex);

    if (render_spot_light* object = dynamic_cast<render_spot_light*>(resolve_id(id)))
    {
        object->set_radius(inner_radius, outer_radius);
    }
    else
    {
        db_warning(renderer, "set_spot_light_radius called with non-existant id {%zi}.", id);
    }
}

std::vector<render_spot_light*> render_scene_manager::get_spot_lights()
{
    std::scoped_lock lock(m_mutex);

    return m_active_spot_lights;
}

// ===========================================================================================
//  Light Probe Grid
// ===========================================================================================

void render_scene_manager::create_light_probe_grid(render_object_id id, const char* name)
{
    std::scoped_lock lock(m_mutex);

    std::unique_ptr<render_light_probe_grid> obj = std::make_unique<render_light_probe_grid>(id, this, m_renderer);
    obj->init();
    obj->set_name(name);

    render_light_probe_grid* obj_ptr = obj.get();

    auto [iter, success] = m_objects.try_emplace(id, std::move(obj));
    if (success)
    {
        m_active_light_probe_grids.push_back(obj_ptr);

        db_verbose(renderer, "Created new light probe grid: {%zi} %s", id, name);
    }
    else
    {
        db_warning(renderer, "render_light_probe_grid called with a duplicate id {%zi}.", id);
    }
}

void render_scene_manager::destroy_light_probe_grid(render_object_id id)
{
    std::scoped_lock lock(m_mutex);

    if (auto iter = m_objects.find(id); iter != m_objects.end())
    {
        db_verbose(renderer, "Removed light probe grid: {%zi} %s", id, iter->second->get_name().c_str());

        m_active_light_probe_grids.erase(std::find(m_active_light_probe_grids.begin(), m_active_light_probe_grids.end(), iter->second.get()));
        m_objects.erase(iter);
    }
    else
    {
        db_warning(renderer, "destroy_light_probe_grid called with non-existant id {%zi}.", id);
    }
}

void render_scene_manager::set_light_probe_grid_density(render_object_id id, float density)
{
    std::scoped_lock lock(m_mutex);

    if (render_light_probe_grid* object = dynamic_cast<render_light_probe_grid*>(resolve_id(id)))
    {
        object->set_density(density);
    }
    else
    {
        db_warning(renderer, "set_light_probe_grid_density called with non-existant id {%zi}.", id);
    }
}

std::vector<render_light_probe_grid*> render_scene_manager::get_light_probe_grid()
{
    std::scoped_lock lock(m_mutex);

    return m_active_light_probe_grids;
}

}; // namespace ws
