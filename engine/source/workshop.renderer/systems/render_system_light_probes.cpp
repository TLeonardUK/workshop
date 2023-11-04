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
#include "workshop.renderer/passes/render_pass_raytracing.h"
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
    const render_options& options = m_renderer.get_options();

    // Grab configuration from renderer.
    m_probe_ray_count = options.light_probe_ray_count;
    m_probe_regenerations_per_frame = options.light_probe_max_regenerations_per_frame;
    m_probe_far_z = options.light_probe_far_z;

    ri_param_block_archetype* scratch_archetype = m_renderer.get_param_block_manager().get_param_block_archetype("raytrace_diffuse_probe_scrach_data");    
    db_assert(scratch_archetype != nullptr);

    // Create scratch buffer to store temporary values.
    ri_buffer::create_params buffer_params;
    buffer_params.element_count = scratch_archetype->get_size();
    buffer_params.element_size = m_probe_ray_count;
    buffer_params.usage = ri_buffer_usage::generic;
    m_scratch_buffer = m_renderer.get_render_interface().create_buffer(buffer_params, "raytrach probe scratch buffer");

    // Create param blocks for regenerating each probe.
    m_regeneration_param_blocks.clear();
    for (size_t i = 0; i < m_probe_regenerations_per_frame; i++)
    {
        std::unique_ptr<ri_param_block> block = m_renderer.get_param_block_manager().create_param_block("raytrace_diffuse_probe_parameters"); 
        m_regeneration_param_blocks.push_back(std::move(block));
    }

    return true;
}

result<void> render_system_light_probes::destroy_resources()
{
    render_scene_manager& scene_manager = m_renderer.get_scene_manager();

    return true;
}

bool render_system_light_probes::is_regenerating()
{
    return !m_dirty_probes.empty();
}

void render_system_light_probes::regenerate()
{
    m_should_regenerate = true;
    m_dirty_probes.clear();
    m_last_dirty_view_position = vector3::zero;

    // Mark all nodes as dirty.
    render_scene_manager& scene_manager = m_renderer.get_scene_manager();
    std::vector<render_light_probe_grid*> probe_grids = scene_manager.get_light_probe_grids();

    for (render_light_probe_grid* grid : probe_grids)
    {
        std::vector<render_light_probe_grid::probe>& probes = grid->get_probes();
        for (render_light_probe_grid::probe& probe : probes)
        {
            probe.dirty = true;
        }
    }
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
    render_system_lighting* lighting_system = m_renderer.get_system<render_system_lighting>();
    
    const render_options& options = m_renderer.get_options();

    if (!options.raytracing_enabled ||
        !m_renderer.get_render_interface().check_feature(ri_feature::raytracing))
    {
        return;
    }

    // Gross: Remove the dependency on this.
    render_view* view = get_main_view();
    if (!view)
    {
        return;
    }

    if (m_probes_to_regenerate.empty())
    {
        return;
    }

    // Add raytracing passes to calculate the probes to regenerate.
    for (size_t i = 0; i < m_probes_to_regenerate.size(); i++)
    {
        dirty_probe& probe = m_probes_to_regenerate[i];  
        ri_param_block* raytrace_diffuse_probe_parameters = m_regeneration_param_blocks[i].get();
        
        raytrace_diffuse_probe_parameters->set("scene_tlas", m_renderer.get_scene_tlas());
        raytrace_diffuse_probe_parameters->set("scene_tlas_metadata", *m_renderer.get_scene_tlas().get_metadata_buffer());
        raytrace_diffuse_probe_parameters->set("probe_origin", probe.probe->origin);
        raytrace_diffuse_probe_parameters->set("probe_far_z", m_probe_far_z);
        raytrace_diffuse_probe_parameters->set("probe_ray_count", (int)m_probe_ray_count);
        raytrace_diffuse_probe_parameters->set("diffuse_output_buffer", probe.grid->get_spherical_harmonic_buffer(), true);
        raytrace_diffuse_probe_parameters->set("diffuse_output_buffer_start_offset", (int)probe.probe->index * (int)render_light_probe_grid::k_probe_coefficient_size);
        raytrace_diffuse_probe_parameters->set("scratch_buffer", *m_scratch_buffer, true);

        // Calculate the scene color for each ray.
        std::unique_ptr<render_pass_raytracing> resolve_pass = std::make_unique<render_pass_raytracing>();
        resolve_pass->name = "raytrace diffuse probe";
        resolve_pass->system = this;
        resolve_pass->dispatch_size = vector3i((int)m_probe_ray_count, 1, 1);
        resolve_pass->technique = m_renderer.get_effect_manager().get_technique("raytrace_diffuse_probe", {});
        resolve_pass->param_blocks.push_back(view->get_view_info_param_block());
        resolve_pass->param_blocks.push_back(raytrace_diffuse_probe_parameters);
        resolve_pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
        resolve_pass->param_blocks.push_back(&lighting_system->get_resolve_param_block(*view));
        graph.add_node(std::move(resolve_pass));

        // Combine and store the resulting probe data.
        std::unique_ptr<render_pass_compute> combine_pass = std::make_unique<render_pass_compute>();
        combine_pass->name = "combine raytrace diffuse probe";
        combine_pass->system = this;
        combine_pass->technique = m_renderer.get_effect_manager().get_technique("raytrace_diffuse_probe_combine", {});
        combine_pass->param_blocks.push_back(view->get_view_info_param_block());
        combine_pass->param_blocks.push_back(raytrace_diffuse_probe_parameters);
        combine_pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
        combine_pass->param_blocks.push_back(&lighting_system->get_resolve_param_block(*view));
        graph.add_node(std::move(combine_pass));

        // Mark probe as not dirty anymore.
        probe.probe->dirty = false;
    }

    db_log(renderer, "Regenerated %zi probes.", m_probes_to_regenerate.size());    
}

render_view* render_system_light_probes::get_main_view()
{
    render_scene_manager& scene_manager = m_renderer.get_scene_manager();    
    std::vector<render_view*> views = scene_manager.get_views();

    for (render_view* view : views)
    {
        if (view->get_flags() == render_view_flags::normal)
        {
            return view;
        }
    }

    return nullptr;
}

void render_system_light_probes::step(const render_world_state& state)
{
    render_scene_manager& scene_manager = m_renderer.get_scene_manager();
    const render_options& options = m_renderer.get_options();

    std::vector<render_view*> views = scene_manager.get_views();
    std::vector<render_light_probe_grid*> probe_grids = scene_manager.get_light_probe_grids();

    m_probes_to_regenerate.clear();

#if 0
    // Debug draw spherical sample points
    render_system_debug* system_debug = m_renderer.get_system<render_system_debug>();
    for (size_t i = 0; i < m_probe_ray_count; i++)
    {
        vector3 n = vector3::spherical_fibonacci((float)i, (float)m_probe_ray_count);
        system_debug->add_line(vector3::zero, n * m_probe_far_z, color::red);
    }
#endif

    // If settings have changed, rebuild resources.
    if (options.light_probe_ray_count != m_probe_ray_count || 
        options.light_probe_max_regenerations_per_frame != m_probe_regenerations_per_frame)
    {
        if (!destroy_resources() || !create_resources())
        {
            db_error(renderer, "Failed to recreate light probe resources.");
        }
    }

    // Look for light probes that need to be updated.
    {
        profile_marker(profile_colors::render, "Light Probe Grids");

        // Build list of dirty probes.
        if (m_dirty_probes.empty() && m_should_regenerate)
        {
            m_should_regenerate = false;

            for (render_light_probe_grid* grid : probe_grids)
            {
                std::vector<render_light_probe_grid::probe>& probes = grid->get_probes();
                for (render_light_probe_grid::probe& probe : probes)
                {
                    if (probe.dirty)
                    {
                        dirty_probe& dirty = m_dirty_probes.emplace_back();
                        dirty.probe = &probe;
                        dirty.grid = grid;
                        dirty.distance = 0.0f;
                    }
                }
            }
        }

        // Sort by distance from camera.
        vector3 view_location = vector3::zero;
        if (render_view* view = get_main_view())
        {
            view_location = view->get_local_location();
        }

        if ((view_location - m_last_dirty_view_position).length() > options.light_probe_queue_update_distance)
        {
            for (dirty_probe& probe : m_dirty_probes)
            {
                probe.distance = (probe.probe->origin - view_location).length_squared();
            }

            std::sort(m_dirty_probes.begin(), m_dirty_probes.end(), [](const dirty_probe& a, const dirty_probe& b) {
                return b.distance < a.distance;
            });

            m_last_dirty_view_position = view_location;
        }

        // If dirty, regenerate this probe.
        while (!m_dirty_probes.empty() && m_probes_to_regenerate.size() < m_probe_regenerations_per_frame)
        {
            dirty_probe probe = m_dirty_probes.back();
            m_dirty_probes.pop_back();

            m_probes_to_regenerate.push_back(probe);
        }
    }
}

}; // namespace ws
