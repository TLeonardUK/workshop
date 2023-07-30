// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_light_probes.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"
#include "workshop.renderer/passes/render_pass_compute.h"
#include "workshop.renderer/passes/render_pass_instanced_model.h"
#include "workshop.renderer/systems/render_system_debug.h"
#include "workshop.renderer/systems/render_system_lighting.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.renderer/render_resource_cache.h"

namespace ws {

render_system_light_probes::render_system_light_probes(renderer& render)
    : render_system(render, "light probes")
{
}

void render_system_light_probes::register_init(init_list& list)
{
    list.add_step(
        "Light Probe Resources",
        [this, &list]() -> result<void> { return create_resources(); },
        [this, &list]() -> result<void> { return destroy_resources(); }
    );
}

result<void> render_system_light_probes::create_resources()
{
    render_scene_manager& scene_manager = m_renderer.get_scene_manager();

    // Create cubemap we will render the scene into.
    ri_texture::create_params params;
    params.width = k_probe_cubemap_size;
    params.height = k_probe_cubemap_size;
    params.depth = 6;
    params.dimensions = ri_texture_dimension::texture_cube;
    params.is_render_target = true;
    params.format = ri_texture_format::R32G32B32A32_FLOAT;

    for (size_t i = 0; i < k_probe_regenerations_per_frame; i++)
    {
        m_probe_capture_targets.push_back(m_renderer.get_render_interface().create_texture(params, "light probe capture target"));
    }

    // Create render views we will use for capturing cubemaps.
    size_t render_views_required = 6 * k_probe_regenerations_per_frame;
    m_probe_capture_views.resize(render_views_required);

    for (size_t probe = 0; probe < k_probe_regenerations_per_frame; probe++)
    {
        for (size_t face = 0; face < 6; face++)
        {
            size_t index = (probe * 6) + face;

            render_object_id view_id = m_renderer.next_render_object_id();
            scene_manager.create_view(view_id, "light probe capture view");

            ri_texture_view rt_view;
            rt_view.texture = m_probe_capture_targets[probe].get();
            rt_view.slice = face;

            render_view* view = scene_manager.resolve_id_typed<render_view>(view_id);
            view->set_render_target(rt_view);
            view->set_viewport({ 0, 0, k_probe_cubemap_size, k_probe_cubemap_size });
            view->set_view_type(render_view_type::custom);
            view->set_view_order(render_view_order::light_probe);
            view->set_clip(k_probe_near_z, k_probe_far_z);
            view->set_should_render(false);
            view->set_flags(render_view_flags::normal | render_view_flags::scene_only | render_view_flags::no_ambient_lighting);

            view_info info;
            info.id = view_id;
            info.resolve_params = m_renderer.get_param_block_manager().create_param_block("calculate_sh_params");

            m_probe_capture_views[index] = std::move(info);
        }
    }

    return true;
}

result<void> render_system_light_probes::destroy_resources()
{
    m_probe_capture_targets.clear();

    return true;
}

void render_system_light_probes::build_graph(render_graph& graph, const render_world_state& state, render_view& view)
{
    if (!view.has_flag(render_view_flags::normal) ||
        view.has_flag(render_view_flags::scene_only))
    {
        return;
    }

    if (m_renderer.get_visualization_mode() != visualization_mode::light_probes)
    {
        return;
    }

    render_scene_manager& scene_manager = m_renderer.get_scene_manager();
    render_system_lighting* lighting_system = m_renderer.get_system<render_system_lighting>();

    // Draw opaque geometry.
    std::unique_ptr<render_pass_instanced_model> pass = std::make_unique<render_pass_instanced_model>();
    pass->name = "light probe debug";
    pass->system = this;
    pass->technique = m_renderer.get_effect_manager().get_technique("light_probe_debug", { });
    pass->render_model = m_renderer.get_debug_model(debug_model::sphere);
    pass->output.color_targets.push_back(&lighting_system->get_lighting_buffer());
    pass->output.depth_target = m_renderer.get_gbuffer_output().depth_target;

    std::vector<render_light_probe_grid*> probe_grids = scene_manager.get_light_probe_grids();
    for (render_light_probe_grid* grid : probe_grids)
    {
        std::vector<render_light_probe_grid::probe>& probes = grid->get_probes();
        for (render_light_probe_grid::probe& probe : probes)
        {
            probe.debug_param_block->set("is_valid", !probe.dirty);
            pass->instances.push_back(probe.debug_param_block.get());
        }
    }

    graph.add_node(std::move(pass));
}

void render_system_light_probes::build_post_graph(render_graph& graph, const render_world_state& state)
{
    for (size_t i = 0; i < m_probes_regenerating; i++)
    {
        view_info& info = m_probe_capture_views[i * 6];

        info.resolve_params->set("cube_texture", *info.render_target);
        info.resolve_params->set("cube_sampler", *m_renderer.get_default_sampler(default_sampler_type::color));
        info.resolve_params->set("cube_size", (int)k_probe_cubemap_size);
        info.resolve_params->set("output_buffer", info.grid->get_spherical_harmonic_buffer(), true);
        info.resolve_params->set("output_start_offset", (int)info.probe_index * (int)render_light_probe_grid::k_probe_coefficient_size);

        // Append a spherical harmonic generation step after the lighting.
        std::unique_ptr<render_pass_compute> cull_pass = std::make_unique<render_pass_compute>();
        cull_pass->name = "compute light probe sh";
        cull_pass->system = this;
        cull_pass->technique = m_renderer.get_effect_manager().get_technique("calculate_sh", {});
        cull_pass->param_blocks.push_back(info.resolve_params.get());
        graph.add_node(std::move(cull_pass));
    }
}

void render_system_light_probes::regenerate_probe(render_light_probe_grid* grid, render_light_probe_grid::probe* probe, size_t regeneration_index)
{
    ri_interface& ri_interface = m_renderer.get_render_interface();
    render_scene_manager& scene_manager = m_renderer.get_scene_manager();
 
    vector3 origin = probe->origin;
    //origin = vector3(0.0f, 200.0f, 0.0f);

    // Which direction each face of our cubemap faces.
    std::array<matrix4, 6> cascade_directions;
    cascade_directions[ri_interface.get_cube_map_face_index(ri_cube_map_face::x_pos)] = matrix4::look_at(origin, origin + vector3(1.0f, 0.0f, 0.0f), vector3(0.0f, 1.0f, 0.0f));
    cascade_directions[ri_interface.get_cube_map_face_index(ri_cube_map_face::x_neg)] = matrix4::look_at(origin, origin + vector3(-1.0f, 0.0f, 0.0f), vector3(0.0f, 1.0f, 0.0f));
    cascade_directions[ri_interface.get_cube_map_face_index(ri_cube_map_face::y_pos)] = matrix4::look_at(origin, origin + vector3(0.0f, 1.0f, 0.0f), vector3(0.0f, 0.0f, -1.0f));
    cascade_directions[ri_interface.get_cube_map_face_index(ri_cube_map_face::y_neg)] = matrix4::look_at(origin, origin + vector3(0.0f, -1.0f, 0.0f), vector3(0.0f, 0.0f, 1.0f));
    cascade_directions[ri_interface.get_cube_map_face_index(ri_cube_map_face::z_pos)] = matrix4::look_at(origin, origin + vector3(0.0f, 0.0f, 1.0f), vector3(0.0f, 1.0f, 0.0f));
    cascade_directions[ri_interface.get_cube_map_face_index(ri_cube_map_face::z_neg)] = matrix4::look_at(origin, origin + vector3(0.0f, 0.0f, -1.0f), vector3(0.0f, 1.0f, 0.0f));

    // Render each side of the face.
    for (size_t i = 0; i < 6; i++)
    {
        matrix4 light_view_matrix = cascade_directions[i];
        matrix4 projection_matrix = matrix4::perspective(
            math::halfpi,
            1.0f,
            k_probe_near_z,
            k_probe_far_z
        );

        size_t view_index = (regeneration_index * 6) + i;
        view_info& info = m_probe_capture_views[view_index];
        info.grid = grid;
        info.probe_index = probe->index;
        info.render_target = m_probe_capture_targets[regeneration_index].get();

        render_view* view = scene_manager.resolve_id_typed<render_view>(info.id);
        view->set_projection_matrix(projection_matrix);
        view->set_view_matrix(light_view_matrix);
        view->set_should_render(true);
        view->set_local_transform(origin, quat::identity, vector3::one);
    }
    
    // Mark probe as not dirty anymore.
    probe->dirty = false;
}

void render_system_light_probes::step(const render_world_state& state)
{
    render_scene_manager& scene_manager = m_renderer.get_scene_manager();

    std::vector<render_light_probe_grid*> probe_grids = scene_manager.get_light_probe_grids();

    m_probes_regenerating = 0;

    // Disable all our views from rendering.
    for (size_t i = 0; i < m_probe_capture_views.size(); i++)
    {
        render_view* view = scene_manager.resolve_id_typed<render_view>(m_probe_capture_views[i].id);
        view->set_should_render(false);
    }

    // Look for light probes that need to be updated.
    {
        profile_marker(profile_colors::render, "Light Probe Grids");

        for (render_light_probe_grid* grid : probe_grids)
        {
            std::vector<render_light_probe_grid::probe>& probes = grid->get_probes();
            for (render_light_probe_grid::probe& probe : probes)
            {
                // If dirty, regenerate this probe.
                if (probe.dirty && m_probes_regenerating < k_probe_regenerations_per_frame)
                {
                    regenerate_probe(grid, &probe, m_probes_regenerating);
                    m_probes_regenerating++;
                }
            }
        }
    }
}

}; // namespace ws
