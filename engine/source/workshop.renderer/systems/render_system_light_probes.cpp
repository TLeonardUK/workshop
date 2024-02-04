// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_light_probes.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_cvars.h"
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

#include "workshop.core/math/random.h"
#include "workshop.core/memory/memory_tracker.h"

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
    
    // Grab configuration from renderer.
    m_probe_ray_count = cvar_light_probe_ray_count.get();
    m_probe_regenerations_per_frame = cvar_light_probe_max_regenerations_per_frame.get();
    m_probe_far_z = cvar_light_probe_far_z.get();

    ri_param_block_archetype* scratch_archetype = m_renderer.get_param_block_manager().get_param_block_archetype("ddgi_probe_scrach_data");    
    db_assert(scratch_archetype != nullptr);

    // Create scratch buffer to store temporary values.
    ri_buffer::create_params buffer_params;
    buffer_params.element_count = scratch_archetype->get_size();
    buffer_params.element_size = m_probe_ray_count * m_probe_regenerations_per_frame;
    buffer_params.usage = ri_buffer_usage::generic;
    m_scratch_buffer = m_renderer.get_render_interface().create_buffer(buffer_params, "raytrach probe scratch buffer");

    // Create param blocks for regenerating each probe.
    m_regeneration_param_block = m_renderer.get_param_block_manager().create_param_block("ddgi_probe_parameters");

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

void render_system_light_probes::build_graph(render_graph& graph, const render_world_state& state, render_view& view)
{
    if (!view.has_flag(render_view_flags::normal) ||
        view.has_flag(render_view_flags::scene_only))
    {
        return;
    }

    if (view.get_visualization_mode() != visualization_mode::light_probes)
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
    pass->param_blocks.push_back(&lighting_system->get_resolve_param_block(view));

    std::vector<render_light_probe_grid*> probe_grids = scene_manager.get_light_probe_grids();
    for (render_light_probe_grid* grid : probe_grids)
    {
        std::vector<render_light_probe_grid::probe>& probes = grid->get_probes();
        for (render_light_probe_grid::probe& probe : probes)
        {
            pass->instances.push_back(probe.debug_param_block.get());
        }
    }

    graph.add_node(std::move(pass));
}

void render_system_light_probes::build_post_graph(render_graph& graph, const render_world_state& state)
{
    profile_marker(profile_colors::render, "build light probe post graph");

    render_system_lighting* lighting_system = m_renderer.get_system<render_system_lighting>();
    
    m_probes_rengerated_last_frame = 0;

    if (!cvar_raytracing_enabled.get() ||
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
        ri_param_block* block = probe.probe->param_block.get();

        size_t table_index, table_offset;
        block->get_table(table_index, table_offset);

        m_regeneration_instance_buffer->add(static_cast<uint32_t>(table_index), static_cast<uint32_t>(table_offset));
    }

    m_regeneration_instance_buffer->commit();

    m_regeneration_param_block->set("scene_tlas", m_renderer.get_scene_tlas());
    m_regeneration_param_block->set("scene_tlas_metadata", *m_renderer.get_scene_tlas().get_metadata_buffer());
    m_regeneration_param_block->set("probe_far_z", m_probe_far_z);
    m_regeneration_param_block->set("probe_ray_count", (int)m_probe_ray_count);
    m_regeneration_param_block->set("scratch_buffer", *m_scratch_buffer, true);
    m_regeneration_param_block->set("probe_data_buffer", m_regeneration_instance_buffer->get_buffer(), false);
    m_regeneration_param_block->set("probe_distance_exponent", cvar_light_probe_distance_exponent.get());
    m_regeneration_param_block->set("probe_blend_hysteresis", cvar_light_probe_blend_hysteresis.get());
    m_regeneration_param_block->set("probe_large_change_threshold", cvar_light_probe_large_change_threshold.get());
    m_regeneration_param_block->set("probe_brightness_threshold", cvar_light_probe_brightness_threshold.get());
    m_regeneration_param_block->set("probe_fixed_ray_backface_threshold", cvar_light_probe_fixed_ray_backface_threshold.get());
    m_regeneration_param_block->set("probe_random_ray_backface_threshold", cvar_light_probe_random_ray_backface_threshold.get());
    m_regeneration_param_block->set("probe_min_frontface_distance", cvar_light_probe_min_frontface_distance.get());
    m_regeneration_param_block->set("random_ray_rotation", vector4(m_random_ray_direction.x, m_random_ray_direction.y, m_random_ray_direction.z, m_random_ray_direction.w));

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
    resolve_pass->technique = m_renderer.get_effect_manager().get_technique("ddgi_trace", {});
    resolve_pass->param_blocks.push_back(view->get_view_info_param_block());
    resolve_pass->param_blocks.push_back(m_regeneration_param_block.get());
    resolve_pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    resolve_pass->param_blocks.push_back(&lighting_system->get_resolve_param_block(*view));
    graph.add_node(std::move(resolve_pass));

    // Output to the irradiance map.
    std::unique_ptr<render_pass_compute> output_irradiance_pass = std::make_unique<render_pass_compute>();
    output_irradiance_pass->name = "ddgi - irradiance output";
    output_irradiance_pass->system = this;
    output_irradiance_pass->technique = m_renderer.get_effect_manager().get_technique("ddgi_output_irradiance", {});
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
    output_occlusion_pass->technique = m_renderer.get_effect_manager().get_technique("ddgi_output_occlusion", {});
    output_occlusion_pass->dispatch_size = vector3i((int)m_probes_to_regenerate.size(), 1, 1);
    output_occlusion_pass->param_blocks.push_back(view->get_view_info_param_block());
    output_occlusion_pass->param_blocks.push_back(m_regeneration_param_block.get());
    output_occlusion_pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    output_occlusion_pass->param_blocks.push_back(&lighting_system->get_resolve_param_block(*view));
    graph.add_node(std::move(output_occlusion_pass));

    // Perform probe relocation to move them out of geometry.
    std::unique_ptr<render_pass_compute> relocate_pass = std::make_unique<render_pass_compute>();
    relocate_pass->name = "ddgi - relocate";
    relocate_pass->system = this;
    relocate_pass->technique = m_renderer.get_effect_manager().get_technique("ddgi_relocate", {});
    relocate_pass->dispatch_size = vector3i((int)m_probes_to_regenerate.size(), 1, 1);
    relocate_pass->param_blocks.push_back(view->get_view_info_param_block());
    relocate_pass->param_blocks.push_back(m_regeneration_param_block.get());
    relocate_pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    relocate_pass->param_blocks.push_back(&lighting_system->get_resolve_param_block(*view));
    graph.add_node(std::move(relocate_pass));

    // Perform probe classification to make invalid probes inactive.
    std::unique_ptr<render_pass_compute> classify_pass = std::make_unique<render_pass_compute>();
    classify_pass->name = "ddgi - classify";
    classify_pass->system = this;
    classify_pass->technique = m_renderer.get_effect_manager().get_technique("ddgi_classify", {});
    classify_pass->dispatch_size = vector3i((int)m_probes_to_regenerate.size(), 1, 1);
    classify_pass->param_blocks.push_back(view->get_view_info_param_block());
    classify_pass->param_blocks.push_back(m_regeneration_param_block.get());
    classify_pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    classify_pass->param_blocks.push_back(&lighting_system->get_resolve_param_block(*view));
    graph.add_node(std::move(classify_pass));

    // End timer.
    std::unique_ptr<render_pass_query> end_query_pass = std::make_unique<render_pass_query>();
    end_query_pass->start = false;
    end_query_pass->query = m_gpu_time_query.get();
    graph.add_node(std::move(end_query_pass));

    m_probes_rengerated_last_frame = m_probes_to_regenerate.size();

    //db_log(renderer, "Regenerated %zi/%zi probes, gpu_time %.2f.", m_probes_to_regenerate.size(), m_adjusted_max_probes_per_frame, m_gpu_time);    
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

    std::vector<render_view*> views = scene_manager.get_views();
    std::vector<render_light_probe_grid*> probe_grids = scene_manager.get_light_probe_grids();

    // Generate a random ray rotation for this frame.
    m_random_ray_direction = random::random_quat();

    // Get the light probe rendering time.
    {
        profile_marker(profile_colors::render, "collect gpu timer");

        if (m_gpu_time_query->are_results_ready())
        {
            m_gpu_time = m_gpu_time_query->get_results() * 1000.0f;

            if (m_probes_rengerated_last_frame > 0 && m_gpu_time > 0.001f)
            {
                // Calculate the average time per frame.
                m_average_gpu_time = (m_average_gpu_time * 0.9f) + (m_gpu_time * 0.1f);

                // Adjust the max probes over time until we converge at a number that 
                // is within the time limit.
                if (m_average_gpu_time < cvar_light_probe_regeneration_time_limit_ms.get())
                {
                    m_adjusted_max_probes_per_frame += cvar_light_probe_regeneration_step_amount.get();
                }
                else if (m_adjusted_max_probes_per_frame > cvar_light_probe_regeneration_step_amount.get())
                {
                    m_adjusted_max_probes_per_frame -= cvar_light_probe_regeneration_step_amount.get();
                }
            }
        }
    }

    // If settings have changed, rebuild resources.
    if (cvar_light_probe_ray_count.get() != m_probe_ray_count || 
        cvar_light_probe_max_regenerations_per_frame.get() != m_probe_regenerations_per_frame)
    {
        if (!destroy_resources() || !create_resources())
        {
            db_error(renderer, "Failed to recreate light probe resources.");
        }
    }

    // Look for light probes that need to be updated.
    {
        m_probes_to_regenerate.clear();

        // Calculate number of probes we can regenerate in our gpu budget.
        size_t probe_regeneration_limit = std::min(m_adjusted_max_probes_per_frame, m_probe_regenerations_per_frame);

        // Build list of probes to regenerate.
        {
            profile_marker(profile_colors::render, "build light probe list");

            double start_time = get_seconds();

            // Create list of frustums that need up to date light probes.
            std::vector<frustum> important_frustums;
            for (render_view* view : views)
            {
                if (view->get_active() && view->get_flags() == render_view_flags::normal)
                {
                    important_frustums.push_back(view->get_frustum());
                }
            }

            // Find the probes that are visible and need to update.
            std::vector<size_t> onscreen_probes;
            std::vector<size_t> offscreen_probes;

            std::vector<dirty_probe> onscreen_dirty_probes;
            std::vector<dirty_probe> offscreen_dirty_probes;

            for (render_light_probe_grid* grid : probe_grids)
            {
                std::vector<render_light_probe_grid::probe>& probes = grid->get_probes();

                onscreen_probes.clear();
                offscreen_probes.clear();
                grid->get_probes_to_update(important_frustums, onscreen_probes, offscreen_probes);

                for (size_t i = 0; i < onscreen_probes.size(); i++)
                {
                    dirty_probe& dirty = onscreen_dirty_probes.emplace_back();
                    dirty.probe = &probes[onscreen_probes[i]];
                    dirty.grid = grid;
                    dirty.distance = 0.0f;
                }

                for (size_t i = 0; i < offscreen_probes.size(); i++)
                {
                    dirty_probe& dirty = offscreen_dirty_probes.emplace_back();
                    dirty.probe = &probes[offscreen_probes[i]];
                    dirty.grid = grid;
                    dirty.distance = 0.0f;
                }
            }

            // Split regeneration limit between onscreen and offscreen probes, with onscreen getting priority.
            size_t offscreen_probe_count = std::min(offscreen_dirty_probes.size(), (size_t)(probe_regeneration_limit * 0.25f));
            size_t onscreen_probe_count = std::min(onscreen_dirty_probes.size(), (size_t)(probe_regeneration_limit * 0.75f));

            size_t remainder = probe_regeneration_limit - (offscreen_probe_count + onscreen_probe_count);
            if (remainder > 0 && onscreen_probe_count < onscreen_dirty_probes.size())
            {
                onscreen_probe_count = std::min(onscreen_dirty_probes.size(), onscreen_probe_count + remainder);
            }

            remainder = probe_regeneration_limit - (offscreen_probe_count + onscreen_probe_count);
            if (remainder > 0 && offscreen_probe_count < offscreen_dirty_probes.size())
            {
                offscreen_probe_count = std::min(offscreen_dirty_probes.size(), offscreen_probe_count + remainder);
            }

            // Build up the regeneration list, use a constantly moving offset to stagger updates.
            for (size_t i = 0; i< offscreen_probe_count; i++)
            {
                m_probes_to_regenerate.push_back(offscreen_dirty_probes[(i + m_offscreen_probe_offset) % offscreen_dirty_probes.size()]);
            }
            m_offscreen_probe_offset += offscreen_probe_count;

            for (size_t i = 0; i < onscreen_probe_count; i++)
            {
                m_probes_to_regenerate.push_back(onscreen_dirty_probes[(i + m_onscreen_probe_offset) % onscreen_dirty_probes.size()]);
            }
            m_onscreen_probe_offset += onscreen_probe_count;
        }
    }
}

}; // namespace ws
