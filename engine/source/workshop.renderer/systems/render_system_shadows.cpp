// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_shadows.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"
#include "workshop.renderer/systems/render_system_debug.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.renderer/objects/render_directional_light.h"
#include "workshop.renderer/objects/render_point_light.h"
#include "workshop.renderer/objects/render_spot_light.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.renderer/render_resource_cache.h"

namespace ws {

float calculate_cascade_split(float nearZ, float farZ, float percent, float blend)
{
	float uni = math::lerp(nearZ, farZ, percent);  
	float log = nearZ * math::pow((farZ / nearZ), percent);
	return math::lerp(uni, log, blend);
}

render_system_shadows::render_system_shadows(renderer& render)
    : render_system(render, "shadows")
{
}

void render_system_shadows::register_init(init_list& list)
{
}

void render_system_shadows::create_graph(render_graph& graph)
{
    // This does't have any independent passes, it instead creates render views in the step.
}

void render_system_shadows::step(const render_world_state& state)
{
    render_scene_manager& scene_manager = m_renderer.get_scene_manager();

    std::vector<render_view*> views = scene_manager.get_views();
    std::vector<render_directional_light*> directional_lights = scene_manager.get_directional_lights();
    std::vector<render_point_light*> point_lights = scene_manager.get_point_lights();
    std::vector<render_spot_light*> spot_lights = scene_manager.get_spot_lights();

    // Erase any cached shadow info that is no longer needed.
    for (auto iter = m_shadow_info.begin(); iter != m_shadow_info.end(); /*empty*/)
    {
        shadow_info& info = *iter;

        // If the combination of light and view is no longer valid we can nuke the shadow data.
        if (scene_manager.resolve_id_typed<render_view>(info.view_id) == nullptr ||
            scene_manager.resolve_id_typed<render_light>(info.light_id) == nullptr)
        {
            // Remove any views these shadows created.
            for (cascade_info& cascade : info.cascades)
            {
                if (cascade.view_id)
                {
                    scene_manager.destroy_view(cascade.view_id);
                }
            }

            iter = m_shadow_info.erase(iter);
        }
        else
        {
            iter++;
        }
    }

    // Render a directional shadow for each view.
    for (render_view* view : views)
    {
        for (render_directional_light* light : directional_lights)
        {
            if (light->get_shodow_casting())
            {
                step_directional_shadow(view, light);
            }
        }
    }

    // Render point lights independently of view.
    for (render_point_light* light : point_lights)
    {
        if (light->get_shodow_casting())
        {
            step_point_shadow(light);
        }
    }

    // Render spot lights independently of view.
    for (render_spot_light* light : spot_lights)
    {
        if (light->get_shodow_casting())
        {
            step_spot_shadow(light);
        }
    }
}

void render_system_shadows::step_view(const render_world_state& state, render_view& view)
{
}

render_system_shadows::shadow_info& render_system_shadows::find_or_create_shadow_info(render_object_id light_id, render_object_id view_id)
{
    for (shadow_info& info : m_shadow_info)
    {
        if (info.light_id == light_id && info.view_id == view_id)
        {
            return info;
        }
    }

    shadow_info& info = m_shadow_info.emplace_back();
    info.light_id = light_id;
    info.view_id = view_id;
    return info;
}

void render_system_shadows::step_directional_shadow(render_view* view, render_directional_light* light)
{
    shadow_info& info = find_or_create_shadow_info(light->get_id(), view->get_id());

    float cascade_near_z;
    float cascade_far_z;
    view->get_clip(cascade_near_z, cascade_far_z);
    cascade_far_z = std::min(cascade_far_z, light->get_shodow_max_distance());

    size_t cascade_count = light->get_shodow_cascades();
    size_t map_size = light->get_shadow_map_size();
    float cascade_exponent = light->get_shodow_cascade_exponent();

    if (!m_renderer.is_rendering_frozen())
    {
        info.view_frustum = view->get_frustum();
        info.view_view_frustum = view->get_view_frustum();
        info.light_rotation = light->get_local_rotation();
    }

    matrix4 rotation_matrix = matrix4::rotation(light->get_local_rotation());
    vector3 vec = -vector3::forward * rotation_matrix;

    matrix4 light_view_matrix = matrix4::look_at(vector3::zero, vector3::forward, vector3::up);

    info.cascades.resize(cascade_count);
    for (size_t i = 0; i < cascade_count; i++)
    {
        cascade_info& cascade = info.cascades[i];

        size_t cascade_map_size = map_size;

        // Last cascade covers a lot larger area so we double the size of the shadow 
        // map to try and keep detail as much as possible.
        if (i == cascade_count - 1)
        {
            cascade_map_size *= 2;
        }

        // Create shadow map if required.
        if (cascade.shadow_map == nullptr || cascade.map_size != cascade_map_size)
        {
            cascade.map_size = cascade_map_size;

            ri_texture::create_params params;
            params.width = cascade.map_size;
            params.height = cascade.map_size;
            params.dimensions = ri_texture_dimension::texture_2d;
            params.is_render_target = true;
            params.format = ri_texture_format::D32_FLOAT;
            cascade.shadow_map = m_renderer.get_render_interface().create_texture(params, "shadow map cascade");
        }

        // Determine shadow map extents.
        float min_delta = (1.0f / cascade_count) * i;
        float max_delta = (1.0f / cascade_count) * (i + 1);

        cascade.split_min_distance = calculate_cascade_split(cascade_near_z, cascade_far_z, min_delta, cascade_exponent);
        cascade.split_max_distance = calculate_cascade_split(cascade_near_z, cascade_far_z, max_delta, cascade_exponent);

        cascade.view_frustum = info.view_frustum.get_cascade(cascade.split_min_distance, cascade.split_max_distance);
        frustum view_space_frustum = info.view_view_frustum.get_cascade(cascade.split_min_distance, cascade.split_max_distance);

        m_renderer.get_system<render_system_debug>()->add_frustum(cascade.view_frustum, color::purple);

        // Calculate bounds of frustum in view space.
        vector3 corners[frustum::k_corner_count];
        view_space_frustum.get_corners(corners);

        aabb bounds(corners, frustum::k_corner_count);

        // Calculate sphere that fits inside the orthographic bounds.
        cascade.world_radius = std::min(bounds.get_extents().x, bounds.get_extents().y);        
        sphere centroid(cascade.view_frustum.get_center() * light_view_matrix, cascade.world_radius);

        m_renderer.get_system<render_system_debug>()->add_sphere(centroid, color::green);

        // Calculate bounding box in light-space.
        aabb light_space_bounds = centroid.get_bounds();

        // Calculate projection matrix that encompases the light space bounds.
        vector3 origin_light_space = info.view_frustum.get_origin() * light_view_matrix;

        // TODO: This seems wrong ... Depth should probably be based on scene bounds?
        float min_z = -origin_light_space.z - 10000.0f;
        float max_z = -origin_light_space.z + 10000.0f;

        cascade.projection_matrix = matrix4::orthographic(
            light_space_bounds.min.x,
            light_space_bounds.max.x,
            light_space_bounds.min.y,
            light_space_bounds.max.y,
            min_z,
            max_z
        );

        // Create the rounding matrix, by projecting the world-space origin and determining the fractional
        // offset in texel space.
        matrix4 shadow_matrix = cascade.projection_matrix * light_view_matrix;
        vector3 shadow_origin = shadow_matrix.transform_location(vector3(0.0f, 0.0f, 0.0f));
        shadow_origin = shadow_origin * cascade.map_size / 2.0f;

        vector3 rounded_origin = shadow_origin.round();
        vector3 rounded_offset = rounded_origin - shadow_origin;
        rounded_offset = rounded_offset * 2.0f / cascade.map_size;
        rounded_offset.z = 0.0f;

        cascade.projection_matrix.set_column(3, cascade.projection_matrix.get_column(3) + vector4(rounded_offset, 0.0f));
 
        // Calculate the frustum from the projection matrix.
        cascade.frustum = frustum(cascade.projection_matrix * light_view_matrix);

        m_renderer.get_system<render_system_debug>()->add_frustum(cascade.frustum, color::red);
    }
}

void render_system_shadows::step_point_shadow(render_point_light* light)
{
    // TODO
}

void render_system_shadows::step_spot_shadow(render_spot_light* light)
{
    // TODO
}

}; // namespace ws
