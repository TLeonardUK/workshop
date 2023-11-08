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
#include "workshop.renderer/passes/render_pass_query.h"
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
    buffer_params.element_size = m_probe_ray_count * m_probe_regenerations_per_frame;
    buffer_params.usage = ri_buffer_usage::generic;
    m_scratch_buffer = m_renderer.get_render_interface().create_buffer(buffer_params, "raytrach probe scratch buffer");

    // Create param blocks for regenerating each probe.
    m_regeneration_param_block = m_renderer.get_param_block_manager().create_param_block("raytrace_diffuse_probe_parameters");

    for (size_t i = 0; i < m_probe_regenerations_per_frame; i++)
    {
        m_probe_param_block.push_back(m_renderer.get_param_block_manager().create_param_block("raytrace_diffuse_probe_data"));
    }

    m_regeneration_instance_buffer = std::make_unique<render_batch_instance_buffer>(m_renderer);

    // Create a query for monitoring gpu time.
    ri_query::create_params query_params;
    query_params.type = ri_query_type::time;
    m_gpu_time_query = m_renderer.get_render_interface().create_query(query_params, "Gpu time query");

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

    m_probes_rengerated_last_frame = 0;

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
    size_t map_padding = 2;

    for (size_t i = 0; i < m_probes_to_regenerate.size(); i++)
    {
        dirty_probe& probe = m_probes_to_regenerate[i];
        ri_param_block* block = m_probe_param_block[i].get();

        block->set("probe_origin", probe.probe->origin);
        block->set("probe_index", (int)probe.probe->index);
        block->set("irradiance_texture", ri_texture_view(&probe.grid->get_irradiance_texture(), 0), true);
        block->set("irradiance_map_size", (int)probe.grid->get_irradiance_map_size());
        block->set("irradiance_per_row", (int)(probe.grid->get_irradiance_texture().get_width() / (probe.grid->get_irradiance_map_size() + map_padding)));
        block->set("occlusion_texture", ri_texture_view(&probe.grid->get_occlusion_texture(), 0), true);
        block->set("occlusion_map_size", (int)probe.grid->get_occlusion_map_size());
        block->set("occlusion_per_row", (int)(probe.grid->get_occlusion_texture().get_width() / (probe.grid->get_occlusion_map_size() + map_padding)));
        block->set("probe_spacing", probe.grid->get_density());

        size_t table_index, table_offset;
        block->get_table(table_index, table_offset);

        m_regeneration_instance_buffer->add(static_cast<uint32_t>(table_index), static_cast<uint32_t>(table_offset));

        // Mark probe as not dirty anymore.
        probe.probe->dirty = false;
    }

    m_regeneration_instance_buffer->commit();

    m_regeneration_param_block->set("scene_tlas", m_renderer.get_scene_tlas());
    m_regeneration_param_block->set("scene_tlas_metadata", *m_renderer.get_scene_tlas().get_metadata_buffer());
    m_regeneration_param_block->set("probe_far_z", m_probe_far_z);
    m_regeneration_param_block->set("probe_ray_count", (int)m_probe_ray_count);
    m_regeneration_param_block->set("scratch_buffer", *m_scratch_buffer, true);
    m_regeneration_param_block->set("probe_data_buffer", m_regeneration_instance_buffer->get_buffer(), false);
    m_regeneration_param_block->set("probe_distance_exponent", options.light_probe_distance_exponent);

    // Start timer.
    std::unique_ptr<render_pass_query> start_query_pass = std::make_unique<render_pass_query>();
    start_query_pass->start = true;
    start_query_pass->query = m_gpu_time_query.get();
    graph.add_node(std::move(start_query_pass));

    // Calculate the scene color for each ray.
    std::unique_ptr<render_pass_raytracing> resolve_pass = std::make_unique<render_pass_raytracing>();
    resolve_pass->name = "ddgi - trace";
    resolve_pass->system = this;
    resolve_pass->dispatch_size = vector3i((int)(m_probe_ray_count * m_probes_to_regenerate.size()), 1, 1);
    resolve_pass->technique = m_renderer.get_effect_manager().get_technique("raytrace_diffuse_probe", {});
    resolve_pass->param_blocks.push_back(view->get_view_info_param_block());
    resolve_pass->param_blocks.push_back(m_regeneration_param_block.get());
    resolve_pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    resolve_pass->param_blocks.push_back(&lighting_system->get_resolve_param_block(*view));
    graph.add_node(std::move(resolve_pass));

    // Output to the irradiance map.
    std::unique_ptr<render_pass_compute> output_irradiance_pass = std::make_unique<render_pass_compute>();
    output_irradiance_pass->name = "ddgi - irradiance output";
    output_irradiance_pass->system = this;
    output_irradiance_pass->technique = m_renderer.get_effect_manager().get_technique("raytrace_diffuse_probe_output_irradiance", {});
    output_irradiance_pass->dispatch_size = vector3i((int)m_probes_to_regenerate.size(), 1, 1);
    output_irradiance_pass->param_blocks.push_back(view->get_view_info_param_block());
    output_irradiance_pass->param_blocks.push_back(m_regeneration_param_block.get());
    output_irradiance_pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    output_irradiance_pass->param_blocks.push_back(&lighting_system->get_resolve_param_block(*view));
    graph.add_node(std::move(output_irradiance_pass));

    // Output to the occlusion map.
    std::unique_ptr<render_pass_compute> output_occlusion_pass = std::make_unique<render_pass_compute>();
    output_occlusion_pass->name = "ddgi - occlusion output";
    output_occlusion_pass->system = this;
    output_occlusion_pass->technique = m_renderer.get_effect_manager().get_technique("raytrace_diffuse_probe_output_occlusion", {});
    output_occlusion_pass->dispatch_size = vector3i((int)m_probes_to_regenerate.size(), 1, 1);
    output_occlusion_pass->param_blocks.push_back(view->get_view_info_param_block());
    output_occlusion_pass->param_blocks.push_back(m_regeneration_param_block.get());
    output_occlusion_pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    output_occlusion_pass->param_blocks.push_back(&lighting_system->get_resolve_param_block(*view));
    graph.add_node(std::move(output_occlusion_pass));

    // Perform relocation
    std::unique_ptr<render_pass_compute> relocate_pass = std::make_unique<render_pass_compute>();
    relocate_pass->name = "ddgi - relocate";
    relocate_pass->system = this;
    relocate_pass->technique = m_renderer.get_effect_manager().get_technique("raytrace_diffuse_probe_relocate", {});
    relocate_pass->dispatch_size = vector3i((int)m_probes_to_regenerate.size(), 1, 1);
    relocate_pass->param_blocks.push_back(view->get_view_info_param_block());
    relocate_pass->param_blocks.push_back(m_regeneration_param_block.get());
    relocate_pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    relocate_pass->param_blocks.push_back(&lighting_system->get_resolve_param_block(*view));
    graph.add_node(std::move(relocate_pass));

    // End timer.
    std::unique_ptr<render_pass_query> end_query_pass = std::make_unique<render_pass_query>();
    end_query_pass->start = false;
    end_query_pass->query = m_gpu_time_query.get();
    graph.add_node(std::move(end_query_pass));

    m_probes_rengerated_last_frame = m_probes_to_regenerate.size();

    db_log(renderer, "Regenerated %zi/%zi probes, gpu_time %.2f.", m_probes_to_regenerate.size(), m_adjusted_max_probes_per_frame, m_gpu_time);    
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

    // Get the light probe rendering time.
    {
        profile_marker(profile_colors::render, "start gpu timer");

        if (m_gpu_time_query->are_results_ready())
        {
            m_gpu_time = m_gpu_time_query->get_results() * 1000.0f;

            if (m_probes_rengerated_last_frame > 0 && m_gpu_time > 0.001f)
            {
                // Calculate the average time per frame.
                m_average_gpu_time = (m_average_gpu_time * 0.9f) + (m_gpu_time * 0.1f);

                // Adjust the max probes over time until we converge at a number that 
                // is within the time limit.
                if (m_average_gpu_time < options.light_probe_regeneration_time_limit_ms)
                {
                    m_adjusted_max_probes_per_frame += options.light_probe_regeneration_step_amount;
                }
                else if (m_adjusted_max_probes_per_frame > options.light_probe_regeneration_step_amount)
                {
                    m_adjusted_max_probes_per_frame -= options.light_probe_regeneration_step_amount;
                }

                db_log(renderer, "gpu_time:%.2f probes:%zi", m_average_gpu_time, m_adjusted_max_probes_per_frame);
            }
        }
    }

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
        size_t probe_regeneration_limit = std::min(m_adjusted_max_probes_per_frame, m_probe_regenerations_per_frame);
        while (!m_dirty_probes.empty() && m_probes_to_regenerate.size() < probe_regeneration_limit)
        {
            dirty_probe probe = m_dirty_probes.back();
            m_dirty_probes.pop_back();

            m_probes_to_regenerate.push_back(probe);
        }
    }
}

}; // namespace ws
