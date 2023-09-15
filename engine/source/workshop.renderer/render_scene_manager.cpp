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
#include "workshop.renderer/objects/render_reflection_probe.h"

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
    std::scoped_lock lock(m_mutex);

    if (auto iter = m_objects.find(id); iter != m_objects.end())
    {
        return iter->second.get();
    }

    return nullptr;
}

std::vector<render_object*> render_scene_manager::get_objects()
{
    std::scoped_lock lock(m_mutex);

    std::vector<render_object*> objects;
    for (auto& [key, value] : m_objects)
    {
        objects.push_back(value.get());
    }

    return objects;
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

void render_scene_manager::set_object_gpu_flags(render_object_id id, render_gpu_flags flags)
{
    std::scoped_lock lock(m_mutex);

    if (render_object* object = resolve_id(id))
    {
        object->set_render_gpu_flags(flags);
    }
    else
    {
        db_warning(renderer, "set_object_gpu_flags called with non-existant id {%zi}.", id);
    }
}

// ===========================================================================================
//  Views
// ===========================================================================================

void render_scene_manager::create_view(render_object_id id, const char* name)
{
    std::scoped_lock lock(m_mutex);

    std::unique_ptr<render_view> view = std::make_unique<render_view>(id, m_renderer);
    view->init();
    view->set_name(name);

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
    std::scoped_lock lock(m_mutex);

    if (auto iter = m_objects.find(id); iter != m_objects.end())
    {
        db_verbose(renderer, "Removed render view: {%zi} %s", id, iter->second->get_name().c_str());

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

    std::unique_ptr<render_static_mesh> obj = std::make_unique<render_static_mesh>(id, m_renderer);
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

void render_scene_manager::set_static_mesh_materials(render_object_id id, const std::vector<asset_ptr<material>>& materials)
{
    std::scoped_lock lock(m_mutex);

    if (render_static_mesh* object = dynamic_cast<render_static_mesh*>(resolve_id(id)))
    {
        object->set_materials(materials);
    }
    else
    {
        db_warning(renderer, "set_static_mesh_materials called with non-existant id {%zi}.", id);
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

    std::unique_ptr<render_directional_light> obj = std::make_unique<render_directional_light>(id, m_renderer);
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

    std::unique_ptr<render_point_light> obj = std::make_unique<render_point_light>(id, m_renderer);
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

    std::unique_ptr<render_spot_light> obj = std::make_unique<render_spot_light>(id, m_renderer);
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

    std::unique_ptr<render_light_probe_grid> obj = std::make_unique<render_light_probe_grid>(id, m_renderer);
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

std::vector<render_light_probe_grid*> render_scene_manager::get_light_probe_grids()
{
    std::scoped_lock lock(m_mutex);

    return m_active_light_probe_grids;
}

// ===========================================================================================
//  Reflection Probes
// ===========================================================================================

void render_scene_manager::create_reflection_probe(render_object_id id, const char* name)
{
    std::scoped_lock lock(m_mutex);

    std::unique_ptr<render_reflection_probe> obj = std::make_unique<render_reflection_probe>(id, m_renderer);
    obj->init();
    obj->set_name(name);

    render_reflection_probe* obj_ptr = obj.get();

    auto [iter, success] = m_objects.try_emplace(id, std::move(obj));
    if (success)
    {
        m_active_reflection_probes.push_back(obj_ptr);

        db_verbose(renderer, "Created new reflection probe: {%zi} %s", id, name);
    }
    else
    {
        db_warning(renderer, "create_reflection_probe called with a duplicate id {%zi}.", id);
    }
}

void render_scene_manager::destroy_reflection_probe(render_object_id id)
{
    std::scoped_lock lock(m_mutex);

    if (auto iter = m_objects.find(id); iter != m_objects.end())
    {
        db_verbose(renderer, "Removed reflection probe: {%zi} %s", id, iter->second->get_name().c_str());

        m_active_reflection_probes.erase(std::find(m_active_reflection_probes.begin(), m_active_reflection_probes.end(), iter->second.get()));
        m_objects.erase(iter);
    }
    else
    {
        db_warning(renderer, "destroy_reflection_probe called with non-existant id {%zi}.", id);
    }
}

std::vector<render_reflection_probe*> render_scene_manager::get_reflection_probes()
{
    std::scoped_lock lock(m_mutex);

    return m_active_reflection_probes;
}


}; // namespace ws
