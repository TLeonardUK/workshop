// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_command_queue.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/systems/render_system_debug.h"

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

void render_command_queue::set_render_flag(render_flag flag, bool value)
{
    queue_command("set_render_flag", [renderer = &m_renderer, flag, value]() {
        renderer->set_render_flag(flag, value);
    });
}

void render_command_queue::toggle_render_flag(render_flag flag)
{
    queue_command("toggle_render_flag", [renderer = &m_renderer, flag]() {
        renderer->set_render_flag(flag, !renderer->get_render_flag(flag));
    });
}

void render_command_queue::regenerate_reflection_probes()
{
    queue_command("regenerate_reflection_probes", [renderer = &m_renderer]() {
        renderer->regenerate_reflection_probes();
    });
}

// ===========================================================================================
//  Debug rendering.
// ===========================================================================================

void render_command_queue::draw_line(const vector3& start, const vector3& end, const color& color)
{
    queue_command("draw_line", [renderer = &m_renderer, start, end, color]() {
        renderer->get_system<render_system_debug>()->add_line(start, end, color);
    });
}

void render_command_queue::draw_aabb(const aabb& bounds, const color& color)
{
    queue_command("draw_aabb", [renderer = &m_renderer, bounds, color]() {
        renderer->get_system<render_system_debug>()->add_aabb(bounds, color);
    });
}

void render_command_queue::draw_obb(const obb& bounds, const color& color)
{
    queue_command("draw_obb", [renderer = &m_renderer, bounds, color]() {
        renderer->get_system<render_system_debug>()->add_obb(bounds, color);
    });
}

void render_command_queue::draw_sphere(const sphere& bounds, const color& color)
{
    queue_command("draw_sphere", [renderer = &m_renderer, bounds, color]() {
        renderer->get_system<render_system_debug>()->add_sphere(bounds, color);
    });
}

void render_command_queue::draw_frustum(const frustum& bounds, const color& color)
{
    queue_command("draw_frustum", [renderer = &m_renderer, bounds, color]() {
        renderer->get_system<render_system_debug>()->add_frustum(bounds, color);
    });
}

void render_command_queue::draw_triangle(const vector3& a, const vector3& b, const vector3& c, const color& color)
{
    queue_command("draw_triangle", [renderer = &m_renderer, a, b, c, color]() {
        renderer->get_system<render_system_debug>()->add_triangle(a, b, c, color);
    });
}

void render_command_queue::draw_cylinder(const cylinder& bounds, const color& color)
{
    queue_command("draw_cylinder", [renderer = &m_renderer, bounds, color]() {
        renderer->get_system<render_system_debug>()->add_cylinder(bounds, color);
    });
}

void render_command_queue::draw_capsule(const cylinder& bounds, const color& color)
{
    queue_command("draw_capsule", [renderer = &m_renderer, bounds, color]() {
        renderer->get_system<render_system_debug>()->add_capsule(bounds, color);
    });
}

void render_command_queue::draw_hemisphere(const hemisphere& bounds, const color& color, bool horizontal_bands)
{
    queue_command("draw_hemisphere", [renderer = &m_renderer, bounds, color, horizontal_bands]() {
        renderer->get_system<render_system_debug>()->add_hemisphere(bounds, color, horizontal_bands);
    });
}

void render_command_queue::draw_cone(const vector3& origin, const vector3& end, float radius, const color& color)
{
    queue_command("draw_cone", [renderer = &m_renderer, origin, end, radius, color]() {
        renderer->get_system<render_system_debug>()->add_cone(origin, end, radius, color);
    });
}

void render_command_queue::draw_arrow(const vector3& start, const vector3& end, const color& color)
{
    queue_command("draw_arrow", [renderer = &m_renderer, start, end, color]() {
        renderer->get_system<render_system_debug>()->add_arrow(start, end, color);
    });
}

void render_command_queue::draw_truncated_cone(const vector3& start, const vector3& end, float start_radius, float end_radius, const color& color)
{
    queue_command("draw_truncated_cone", [renderer = &m_renderer, start, end, start_radius, end_radius, color]() {
        renderer->get_system<render_system_debug>()->add_truncated_cone(start, end, start_radius, end_radius, color);
    });
}

// ===========================================================================================
//  Worlds
// ===========================================================================================

render_object_id render_command_queue::create_world(const char* name)
{
    render_object_id id = m_renderer.next_render_object_id();
    const char* stored_name = allocate_copy(name);

    queue_command("create_world", [renderer = &m_renderer, id, stored_name]() {
        renderer->get_scene_manager().create_world(id, stored_name);
    });

    return id;
}

void render_command_queue::destroy_world(render_object_id id)
{
    queue_command("destroy_world", [renderer = &m_renderer, id]() {
        renderer->get_scene_manager().destroy_world(id);
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

void render_command_queue::set_object_gpu_flags(render_object_id id, render_gpu_flags flags)
{
    queue_command("set_object_gpu_flags", [renderer = &m_renderer, id, flags]() {
        renderer->get_scene_manager().set_object_gpu_flags(id, flags);
    });
}

void render_command_queue::set_object_draw_flags(render_object_id id, render_draw_flags flags)
{
    queue_command("set_object_draw_flags", [renderer = &m_renderer, id, flags]() {
        renderer->get_scene_manager().set_object_draw_flags(id, flags);
    });
}

void render_command_queue::set_object_visibility(render_object_id id, bool visibility)
{
    queue_command("set_object_visibility", [renderer = &m_renderer, id, visibility]() {
        renderer->get_scene_manager().set_object_visibility(id, visibility);
    });
}

void render_command_queue::set_object_world(render_object_id id, render_object_id world_id)
{
    queue_command("set_object_world", [renderer = &m_renderer, id, world_id]() {
        renderer->get_scene_manager().set_object_world(id, world_id);
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

void render_command_queue::set_view_readback_pixmap(render_object_id id, pixmap* output)
{
    queue_command("set_view_readback_pixmap", [renderer = &m_renderer, id, output]() {
        renderer->get_scene_manager().set_view_readback_pixmap(id, output);
    });
}

// ================================================================================================
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

void render_command_queue::set_static_mesh_materials(render_object_id id, const std::vector<asset_ptr<material>>& materials)
{
    queue_command("set_static_mesh_materials", [renderer = &m_renderer, id, materials]() {
        renderer->get_scene_manager().set_static_mesh_materials(id, materials);
    });
}

// ================================================================================================
//  Lights
// ================================================================================================

void render_command_queue::set_light_intensity(render_object_id id, float value)
{
    queue_command("set_light_intensity", [renderer = &m_renderer, id, value]() {
        renderer->get_scene_manager().set_light_intensity(id, value);
    });
}

void render_command_queue::set_light_range(render_object_id id, float value)
{
    queue_command("set_light_range", [renderer = &m_renderer, id, value]() {
        renderer->get_scene_manager().set_light_range(id, value);
    });
}

void render_command_queue::set_light_importance_distance(render_object_id id, float value)
{
    queue_command("set_light_importance_distance", [renderer = &m_renderer, id, value]() {
        renderer->get_scene_manager().set_light_importance_distance(id, value);
    });
}

void render_command_queue::set_light_color(render_object_id id, color color)
{
    queue_command("set_light_color", [renderer = &m_renderer, id, color]() {
        renderer->get_scene_manager().set_light_color(id, color);
    });
}

void render_command_queue::set_light_shadow_casting(render_object_id id, bool value)
{
    queue_command("set_light_shadow_casting", [renderer = &m_renderer, id, value]() {
        renderer->get_scene_manager().set_light_shadow_casting(id, value);
    });
}

void render_command_queue::set_light_shadow_map_size(render_object_id id, size_t value)
{
    queue_command("set_light_shadow_map_size", [renderer = &m_renderer, id, value]() {
        renderer->get_scene_manager().set_light_shadow_map_size(id, value);
    });
}

void render_command_queue::set_light_shadow_max_distance(render_object_id id, float value)
{
    queue_command("set_light_shadow_max_distance", [renderer = &m_renderer, id, value]() {
        renderer->get_scene_manager().set_light_shadow_max_distance(id, value);
    });
}

// ================================================================================================
//  Directional lights
// ================================================================================================

render_object_id render_command_queue::create_directional_light(const char* name)
{
    render_object_id id = m_renderer.next_render_object_id();
    const char* stored_name = allocate_copy(name);

    queue_command("create_directional_light", [renderer = &m_renderer, id, stored_name]() {
        renderer->get_scene_manager().create_directional_light(id, stored_name);
    });

    return id;
}

void render_command_queue::destroy_directional_light(render_object_id id)
{
    queue_command("destroy_directional_light", [renderer = &m_renderer, id]() {
        renderer->get_scene_manager().destroy_directional_light(id);
    });
}

void render_command_queue::set_directional_light_shadow_cascades(render_object_id id, size_t value)
{
    queue_command("set_directional_light_shadow_cascades", [renderer = &m_renderer, id, value]() {
        renderer->get_scene_manager().set_directional_light_shadow_cascades(id, value);
    });
}

void render_command_queue::set_directional_light_shadow_cascade_exponent(render_object_id id, float value)
{
    queue_command("set_directional_light_shadow_cascades", [renderer = &m_renderer, id, value]() {
        renderer->get_scene_manager().set_directional_light_shadow_cascade_exponent(id, value);
    });
}

void render_command_queue::set_directional_light_shadow_cascade_blend(render_object_id id, float value)
{
    queue_command("set_directional_light_shadow_cascade_blend", [renderer = &m_renderer, id, value]() {
        renderer->get_scene_manager().set_directional_light_shadow_cascade_blend(id, value);
    });
}

// ================================================================================================
//  Point lights
// ================================================================================================

render_object_id render_command_queue::create_point_light(const char* name)
{
    render_object_id id = m_renderer.next_render_object_id();
    const char* stored_name = allocate_copy(name);

    queue_command("create_point_light", [renderer = &m_renderer, id, stored_name]() {
        renderer->get_scene_manager().create_point_light(id, stored_name);
    });

    return id;
}

void render_command_queue::destroy_point_light(render_object_id id)
{
    queue_command("destroy_point_light", [renderer = &m_renderer, id]() {
        renderer->get_scene_manager().destroy_point_light(id);
    });
}

// ================================================================================================
//  Spot lights
// ================================================================================================

render_object_id render_command_queue::create_spot_light(const char* name)
{
    render_object_id id = m_renderer.next_render_object_id();
    const char* stored_name = allocate_copy(name);

    queue_command("create_spot_light", [renderer = &m_renderer, id, stored_name]() {
        renderer->get_scene_manager().create_spot_light(id, stored_name);
    });

    return id;
}

void render_command_queue::destroy_spot_light(render_object_id id)
{
    queue_command("destroy_spot_light", [renderer = &m_renderer, id]() {
        renderer->get_scene_manager().destroy_spot_light(id);
    });
}

void render_command_queue::set_spot_light_radius(render_object_id id, float inner_radius, float outer_radius)
{
    queue_command("set_spot_light_radius", [renderer = &m_renderer, id, inner_radius, outer_radius]() {
        renderer->get_scene_manager().set_spot_light_radius(id, inner_radius, outer_radius);
    });
}

// ===========================================================================================
//  Light Probe Grid
// ===========================================================================================

render_object_id render_command_queue::create_light_probe_grid(const char* name)
{
    render_object_id id = m_renderer.next_render_object_id();
    const char* stored_name = allocate_copy(name);

    queue_command("create_light_probe_grid", [renderer = &m_renderer, id, stored_name]() {
        renderer->get_scene_manager().create_light_probe_grid(id, stored_name);
    });

    return id;
}

void render_command_queue::destroy_light_probe_grid(render_object_id id)
{
    queue_command("destroy_light_probe_grid", [renderer = &m_renderer, id]() {
        renderer->get_scene_manager().destroy_light_probe_grid(id);
    });
}

void render_command_queue::set_light_probe_grid_density(render_object_id id, float density)
{
    queue_command("set_light_probe_grid_density", [renderer = &m_renderer, id, density]() {
        renderer->get_scene_manager().set_light_probe_grid_density(id, density);
    });
}

// ===========================================================================================
//  Reflection Probe
// ===========================================================================================

render_object_id render_command_queue::create_reflection_probe(const char* name)
{
    render_object_id id = m_renderer.next_render_object_id();
    const char* stored_name = allocate_copy(name);

    queue_command("create_reflection_probe", [renderer = &m_renderer, id, stored_name]() {
        renderer->get_scene_manager().create_reflection_probe(id, stored_name);
    });

    return id;
}

void render_command_queue::destroy_reflection_probe(render_object_id id)
{
    queue_command("destroy_reflection_probe", [renderer = &m_renderer, id]() {
        renderer->get_scene_manager().destroy_reflection_probe(id);
    });
}

}; // namespace ws
