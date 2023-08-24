// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_shadows.h"
#include "workshop.renderer/systems/render_system_debug.h"
#include "workshop.renderer/systems/render_system_light_probes.h"
#include "workshop.renderer/systems/render_system_reflection_probes.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"
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
    // These systems make activate/deactivate views which will change what
    // shadows we need to render.
    add_dependency<render_system_light_probes>();
    add_dependency<render_system_reflection_probes>();
}

void render_system_shadows::build_graph(render_graph& graph, const render_world_state& state, render_view& view)
{
    // This does't have any independent passes, it instead creates render views in the step.
}

void render_system_shadows::step(const render_world_state& state)
{
    render_scene_manager& scene_manager = m_renderer.get_scene_manager();

    const render_options& options = m_renderer.get_options();

    std::vector<render_view*> views = scene_manager.get_views();
    std::vector<render_directional_light*> directional_lights = scene_manager.get_directional_lights();
    std::vector<render_point_light*> point_lights = scene_manager.get_point_lights();
    std::vector<render_spot_light*> spot_lights = scene_manager.get_spot_lights();

    // Erase any cached shadow info that is no longer needed.
    {
        profile_marker(profile_colors::render, "Cascade Cleanup");

        for (auto iter = m_shadow_info.begin(); iter != m_shadow_info.end(); /*empty*/)
        {
            shadow_info& info = *iter;

            render_view* view = scene_manager.resolve_id_typed<render_view>(info.view_id);

            // If the combination of light and view is no longer valid we can nuke the shadow data.
            if ((info.view_id > 0 && view == nullptr) ||
                (info.light_id > 0 && scene_manager.resolve_id_typed<render_light>(info.light_id) == nullptr))
            {
                // Remove any views these shadows created.
                for (cascade_info& cascade : info.cascades)
                {
                    destroy_cascade(cascade);
                }

                iter = m_shadow_info.erase(iter);
            }
            else
            {
                // Clean up any shadow maps of inactive views, they no longer require their maps to be cached so
                // we can purge them to keep memory usage down.
                if (view && !view->get_active())
                {
                    for (cascade_info& cascade : info.cascades)
                    {
                        cascade.shadow_map = nullptr;
                        cascade.shadow_map_view.texture = nullptr;
                    }
                }

                iter++;
            }
        }
    }

    // Render a directional shadow for each view.
    {
        profile_marker(profile_colors::render, "Directional Lights");

        for (render_view* view : views)
        {
            if (!view->get_active())
            {
                continue;
            }

            if ((view->get_flags() & render_view_flags::normal) == (render_view_flags)0)
            {
                continue;
            }

            for (render_directional_light* light : directional_lights)
            {
                if (light->get_shodow_casting())
                {
                    step_directional_shadow(view, light);
                }
            }
        }
    }

    // Render point lights independently of view.
    {
        profile_marker(profile_colors::render, "Point Lights");

        for (render_point_light* light : point_lights)
        {
            if (light->get_shodow_casting())
            {
                step_point_shadow(nullptr, light);
            }
        }
    }

    // Render spot lights independently of view.
    {
        profile_marker(profile_colors::render, "Spot Lights");

        for (render_spot_light* light : spot_lights)
        {
            if (light->get_shodow_casting())
            {
                step_spot_shadow(nullptr, light);
            }
        }
    }

    // Update all cascades.
    std::vector<cascade_info*> cascades_needing_render;
    {        
        profile_marker(profile_colors::render, "Step Cascades");

        for (shadow_info& info : m_shadow_info)
        {
            render_view* base_view = scene_manager.resolve_id_typed<render_view>(info.view_id);
            bool base_view_is_active = (base_view == nullptr || base_view->get_active());

            for (cascade_info& cascade : info.cascades)
            {
                render_view* cascade_view = scene_manager.resolve_id_typed<render_view>(cascade.view_id);
                if (cascade_view)
                {
                    cascade_view->set_active(base_view_is_active);
                }

                // We can skip all of this if the parent view is currently inactive.
                if (!base_view_is_active && cascade.view_id)
                {
                    cascade_view->set_should_render(false);
                    cascade_view->set_active(false);
                    continue;
                }

                step_cascade(info, cascade);

                if (cascade.needs_render)
                {
                    cascades_needing_render.push_back(&cascade);
                }
                else if (cascade.view_id)
                {
                    cascade_view->set_should_render(false);
                    cascade_view->set_active(false);
                }
            }
        }
    }

    // Grab all cascades that need an update, and sort by last time they updated, then
    // mark the top x as should update. Spreads out any large updates over a few frames.
    // TODO: Cubemaps should be updated all at once or we get some really nasty problems.

    size_t frame_index = state.time.frame_count;
    {
        profile_marker(profile_colors::render, "Cascade Sort");

        std::sort(cascades_needing_render.begin(), cascades_needing_render.end(), [frame_index](const cascade_info* a, const cascade_info* b) {
            size_t frames_elapsed_a = (frame_index - a->last_rendered_frame);
            size_t frames_elapsed_b = (frame_index - b->last_rendered_frame);
            return frames_elapsed_b < frames_elapsed_a;
        });
    }

    {
        profile_marker(profile_colors::render, "Update Cascade Params");

        for (size_t i = 0; i < cascades_needing_render.size(); i++)
        {
            cascade_info* cascade = cascades_needing_render[i];
            bool should_render = (i < options.shadows_max_cascade_updates_per_frame);

            if (cascade->view_id)
            {
                render_view* view = scene_manager.resolve_id_typed<render_view>(cascade->view_id);
                view->set_should_render(should_render);

                if (should_render)
                {
                    cascade->needs_render = false;
                    cascade->last_rendered_frame = frame_index;

                    update_cascade_param_block(*cascade);
                }
            }
        }
    }
}

render_system_shadows::shadow_info& render_system_shadows::find_or_create_shadow_info(render_object_id light_id, render_object_id view_id)
{
    std::scoped_lock lock(m_shadow_info_mutex);

    for (shadow_info& info : m_shadow_info)
    {
        if (info.light_id == light_id && (info.view_id == view_id || info.view_id == 0))
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
    cascade_far_z = std::min(cascade_far_z, light->get_shadow_max_distance());

    size_t cascade_count = light->get_shodow_cascades();
    size_t map_size = light->get_shadow_map_size();
    float cascade_exponent = light->get_shodow_cascade_exponent();

    if (!m_renderer.get_render_flag(render_flag::freeze_rendering))
    {
        info.view_frustum = view->get_frustum();
        info.view_view_frustum = view->get_view_frustum();
        info.light_rotation = light->get_local_rotation();
    }

    matrix4 rotation_matrix = matrix4::rotation(info.light_rotation);
    matrix4 light_view_matrix = matrix4::look_at(vector3::zero, vector3::forward * rotation_matrix.inverse(), vector3::up);

    // Destroy any cascades we are stripping off.
    if (info.cascades.size() > cascade_count)
    {
        for (size_t i = info.cascades.size(); i < cascade_count; i++)
        {
            destroy_cascade(info.cascades[i]);
        }
    }

    info.cascades.resize(cascade_count);
    for (size_t i = 0; i < cascade_count; i++)
    {
        cascade_info& cascade = info.cascades[i];

        size_t cascade_map_size = map_size;

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
            cascade.shadow_map = m_renderer.get_render_interface().create_texture(params, "directional shadow map cascade");
            cascade.shadow_map_view = cascade.shadow_map.get();
        }

        // Determine shadow map extents.
        float min_delta = (1.0f / cascade_count) * i;
        float max_delta = (1.0f / cascade_count) * (i + 1);

        cascade.split_min_distance = calculate_cascade_split(cascade_near_z, cascade_far_z, min_delta, cascade_exponent);
        cascade.split_max_distance = calculate_cascade_split(cascade_near_z, cascade_far_z, max_delta, cascade_exponent);
        cascade.view_frustum = info.view_frustum.get_cascade(cascade.split_min_distance, cascade.split_max_distance);
        cascade.blend_factor = light->get_shodow_cascade_blend();

        // Calculate bounds of frustum in light space so the radius won't change
        // as we rotate the view.
        frustum view_view_frustum = info.view_view_frustum.get_cascade(cascade.split_min_distance, cascade.split_max_distance);;

        vector3 corners[frustum::k_corner_count];
        view_view_frustum.get_corners(corners);
        aabb bounds(corners, frustum::k_corner_count);

        // Calculate sphere that fits inside the frustum.
        cascade.world_radius = std::max(bounds.get_extents().x, bounds.get_extents().y) * 2.0f;        
        sphere centroid(cascade.view_frustum.get_center() * light_view_matrix, cascade.world_radius);

        // Calculate bounding box in light-space.
        aabb light_space_bounds = centroid.get_bounds();

        // Calculate projection matrix that encompases the light space bounds.
        vector3 origin_light_space = info.view_frustum.get_origin() * light_view_matrix;

        // TODO: This seems wrong ... Depth should probably be based on scene bounds?
        float min_z = origin_light_space.z - 10000.0f;
        float max_z = origin_light_space.z + 10000.0f;

        cascade.view_matrix = light_view_matrix;
        cascade.projection_matrix = matrix4::orthographic(
            light_space_bounds.min.x,
            light_space_bounds.max.x,
            light_space_bounds.min.y,
            light_space_bounds.max.y,
            min_z,
            max_z
        );
        cascade.z_near = min_z;
        cascade.z_far = max_z;

        // Create the rounding matrix, by projecting the world-space origin and determining the fractional
        // offset in texel space.
        // We do this to avoid shimmering as the camera moves, we always want to move in texel steps.
        matrix4 shadow_matrix = light_view_matrix * cascade.projection_matrix;
        vector3 shadow_origin = shadow_matrix.transform_location(vector3(0.0f, 0.0f, 0.0f));
        shadow_origin = shadow_origin * static_cast<float>(cascade.map_size) / 2.0f;

        vector3 rounded_origin = shadow_origin.round();
        vector3 rounded_offset = rounded_origin - shadow_origin;
        rounded_offset = rounded_offset * 2.0f / static_cast<float>(cascade.map_size);
        rounded_offset.z = 0.0f;

        cascade.projection_matrix[0][3] = cascade.projection_matrix[0][3] + rounded_offset.x;
        cascade.projection_matrix[1][3] = cascade.projection_matrix[1][3] + rounded_offset.y;
    }
}

void render_system_shadows::step_point_shadow(render_view* view, render_point_light* light)
{
    shadow_info& info = find_or_create_shadow_info(light->get_id(), 0);

    // TODO: We should expose this somewhere.
    float cascade_near_z = 10.0f;
    float cascade_far_z = std::min(light->get_shadow_max_distance(), light->get_range());

    float range = light->get_range();

    if (!m_renderer.get_render_flag(render_flag::freeze_rendering))
    {
        info.light_rotation = light->get_local_rotation();
        info.light_location = light->get_local_location();
    }

    // Which direction each face of our cubemap faces.
    std::array<matrix4, 6> cascade_directions;
    ri_interface& ri_interface = m_renderer.get_render_interface();
    cascade_directions[ri_interface.get_cube_map_face_index(ri_cube_map_face::x_pos)] = matrix4::look_at(info.light_location, info.light_location + vector3(1.0f, 0.0f, 0.0f),  vector3(0.0f, 1.0f, 0.0f));
    cascade_directions[ri_interface.get_cube_map_face_index(ri_cube_map_face::x_neg)] = matrix4::look_at(info.light_location, info.light_location + vector3(-1.0f, 0.0f, 0.0f), vector3(0.0f, 1.0f, 0.0f));
    cascade_directions[ri_interface.get_cube_map_face_index(ri_cube_map_face::y_pos)] = matrix4::look_at(info.light_location, info.light_location + vector3(0.0f, 1.0f, 0.0f),  vector3(0.0f, 0.0f, -1.0f));
    cascade_directions[ri_interface.get_cube_map_face_index(ri_cube_map_face::y_neg)] = matrix4::look_at(info.light_location, info.light_location + vector3(0.0f, -1.0f, 0.0f), vector3(0.0f, 0.0f, 1.0f));
    cascade_directions[ri_interface.get_cube_map_face_index(ri_cube_map_face::z_pos)] = matrix4::look_at(info.light_location, info.light_location + vector3(0.0f, 0.0f, 1.0f),  vector3(0.0f, 1.0f, 0.0f));
    cascade_directions[ri_interface.get_cube_map_face_index(ri_cube_map_face::z_neg)] = matrix4::look_at(info.light_location, info.light_location + vector3(0.0f, 0.0f, -1.0f), vector3(0.0f, 1.0f, 0.0f));

    info.cascades.resize(6);
    for (size_t i = 0; i < 6; i++)
    {
        cascade_info& cascade = info.cascades[i];

        // Create shadow map if required.
        if (i == 0)
        {
            if (cascade.shadow_map == nullptr || cascade.map_size != light->get_shadow_map_size())
            {
                cascade.map_size = light->get_shadow_map_size();

                ri_texture::create_params params;
                params.width = cascade.map_size;
                params.height = cascade.map_size;
                params.depth = 6;
                params.dimensions = ri_texture_dimension::texture_cube;
                params.is_render_target = true;
                params.format = ri_texture_format::D32_FLOAT;

                cascade.shadow_map = m_renderer.get_render_interface().create_texture(params, "point shadow map cascade");
                cascade.shadow_map_view.texture = cascade.shadow_map.get();
                cascade.shadow_map_view.slice = i;
            }
        }
        else
        {
            cascade.shadow_map_view.texture = info.cascades[0].shadow_map.get();
            cascade.shadow_map_view.slice = i;
            cascade.map_size = info.cascades[0].map_size;
        }
        
        //matrix4 light_view_matrix = matrix4::look_at(info.light_location, info.light_location + cascade_directions[i], vector3::up);
        matrix4 light_view_matrix = cascade_directions[i];

        // Setup an appropriate matrix to capture the shadow map.
        cascade.view_matrix = light_view_matrix;
        cascade.projection_matrix = matrix4::perspective(
            math::halfpi,
            1.0f,
            cascade_near_z,
            cascade_far_z
        );
        cascade.z_near = cascade_near_z;
        cascade.z_far = cascade_far_z;
        cascade.use_linear_depth = true;

        // Calculate the frustum from the projection matrix.
        cascade.frustum = frustum(light_view_matrix * cascade.projection_matrix);
    }
}

void render_system_shadows::step_spot_shadow(render_view* view, render_spot_light* light)
{
    shadow_info& info = find_or_create_shadow_info(light->get_id(), 0);

    // TODO: We should expose this somewhere.
    float cascade_near_z = 10.0f;
    float cascade_far_z = std::min(light->get_shadow_max_distance(), light->get_range());

    float inner_radius = 0.0f;
    float outer_radius = 0.0f;
    light->get_radius(inner_radius, outer_radius);

    if (!m_renderer.get_render_flag(render_flag::freeze_rendering))
    {
        info.light_rotation = light->get_local_rotation();
        info.light_location = light->get_local_location();
    }

    matrix4 rotation_matrix = matrix4::rotation(info.light_rotation);
    matrix4 light_view_matrix = matrix4::look_at(info.light_location, info.light_location + (vector3::forward * rotation_matrix.inverse()), vector3::up);

    info.cascades.resize(1);
    cascade_info& cascade = info.cascades[0];

    // Create shadow map if required.
    if (cascade.shadow_map == nullptr || cascade.map_size != light->get_shadow_map_size())
    {
        cascade.map_size = light->get_shadow_map_size();

        ri_texture::create_params params;
        params.width = cascade.map_size;
        params.height = cascade.map_size;
        params.dimensions = ri_texture_dimension::texture_2d;
        params.is_render_target = true;
        params.format = ri_texture_format::D32_FLOAT;
        cascade.shadow_map = m_renderer.get_render_interface().create_texture(params, "spotlight shadow map cascade");
        cascade.shadow_map_view = cascade.shadow_map.get();
    }

    // Setup an appropriate matrix to capture the shadow map.
    cascade.view_matrix = light_view_matrix;
    cascade.projection_matrix = matrix4::perspective(
        outer_radius * 2.5f,
        1.0f,
        cascade_near_z,
        cascade_far_z
    );
    cascade.z_near = cascade_near_z;
    cascade.z_far = cascade_far_z;

    // Calculate the frustum from the projection matrix.
    cascade.frustum = frustum(light_view_matrix * cascade.projection_matrix);
}

void render_system_shadows::destroy_cascade(cascade_info& info)
{
    render_scene_manager& scene_manager = m_renderer.get_scene_manager();
    
    if (info.view_id)
    {
        scene_manager.destroy_view(info.view_id);
    }
}

void render_system_shadows::step_cascade(shadow_info& info, cascade_info& cascade)
{
    render_scene_manager& scene_manager = m_renderer.get_scene_manager();

    if (!cascade.view_id)
    {
        cascade.view_id = m_renderer.next_render_object_id();
        scene_manager.create_view(cascade.view_id, "Shadow Cascade View");

        cascade.shadow_map_state_param_block = m_renderer.get_param_block_manager().create_param_block("shadow_map_state");
        update_cascade_param_block(cascade);
    }

    render_view* parent_view = scene_manager.resolve_id_typed<render_view>(info.view_id);

    render_view* view = scene_manager.resolve_id_typed<render_view>(cascade.view_id);
    view->set_projection_matrix(cascade.projection_matrix);
    view->set_view_matrix(cascade.view_matrix);
    view->set_render_target(cascade.shadow_map_view);
    view->set_viewport(recti(0, 0, (int)cascade.map_size, (int)cascade.map_size));
    if (cascade.use_linear_depth)
    {
        view->set_flags(render_view_flags::linear_depth_only);
    }
    else
    {
        view->set_flags(render_view_flags::depth_only);
    }
    view->set_view_type(render_view_type::custom);
    view->set_view_order(render_view_order::shadows);
    view->set_clip(cascade.z_near, cascade.z_far);
    view->set_local_transform(info.light_location, quat::identity, vector3::one);

    if (view->has_view_changed() || (parent_view && parent_view->has_view_changed()))
    {
        cascade.needs_render = true;
    }    
}

void render_system_shadows::update_cascade_param_block(cascade_info& cascade)
{
    cascade.shadow_map_state_param_block->set("shadow_matrix", cascade.view_matrix * cascade.projection_matrix);
    cascade.shadow_map_state_param_block->set("depth_map", *cascade.shadow_map_view.texture);
    cascade.shadow_map_state_param_block->set("depth_map_size", (int)cascade.map_size);
    cascade.shadow_map_state_param_block->set("z_far", cascade.z_far);
    cascade.shadow_map_state_param_block->set("z_near", cascade.z_near);
}

}; // namespace ws
