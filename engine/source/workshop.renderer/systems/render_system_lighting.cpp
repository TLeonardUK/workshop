// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_lighting.h"
#include "workshop.renderer/systems/render_system_shadows.h"
#include "workshop.renderer/systems/render_system_debug.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"
#include "workshop.renderer/passes/render_pass_compute.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.renderer/objects/render_directional_light.h"
#include "workshop.renderer/objects/render_point_light.h"
#include "workshop.renderer/objects/render_spot_light.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.renderer/render_resource_cache.h"

namespace ws {

render_system_lighting::render_system_lighting(renderer& render)
    : render_system(render, "lighting")
{
}

void render_system_lighting::register_init(init_list& list)
{
    list.add_step(
        "Renderer Resources",
        [this, &list]() -> result<void> { return create_resources(); },
        [this, &list]() -> result<void> { return destroy_resources(); }
    );
}

result<void> render_system_lighting::create_resources()
{
    ri_interface& render_interface = m_renderer.get_render_interface();

    // Output buffer for the main lighting pass.
    ri_texture::create_params texture_params;
    texture_params.width = m_renderer.get_display_width();
    texture_params.height = m_renderer.get_display_height();
    texture_params.dimensions = ri_texture_dimension::texture_2d;
    texture_params.format = ri_texture_format::R16G16B16A16_FLOAT;
    texture_params.is_render_target = true;
    m_lighting_buffer = render_interface.create_texture(texture_params, "lighting buffer");
    
    m_lighting_output.depth_target = nullptr;
    m_lighting_output.color_targets.push_back(m_lighting_buffer.get());

    return true;
}

result<void> render_system_lighting::destroy_resources()
{
    return true;
}

ri_texture& render_system_lighting::get_lighting_buffer()
{
    return *m_lighting_buffer;
}

/*
void render_system_lighting::cluster_lights(render_view& view, const std::vector<render_light*>& lights, cluster_list& clusters)
{
    if (!m_renderer.is_rendering_frozen())
    {
        m_cluster_prime_frustum = view.get_frustum();        
        view.get_clip(m_cluster_prime_near_z, m_cluster_prime_far_z);
    }

    m_renderer.get_system<render_system_debug>()->add_frustum(m_cluster_prime_frustum, color::green);

    // Calculate bounds up front.
    for (size_t z = 0; z < k_cluster_dimensions; z++)
    {
        for (size_t y = 0; y < k_cluster_dimensions; y++)
        {
            for (size_t x = 0; x < k_cluster_dimensions; x++)
            {
                cluster& cluster = clusters.get(x, y, z);

                float range_z = (m_cluster_prime_far_z - m_cluster_prime_near_z);
                float step_z = range_z / k_cluster_dimensions;
                float cluster_near_z = m_cluster_prime_near_z + (step_z * z);
                float cluster_far_z = m_cluster_prime_near_z + step_z;

                m_cluster_prime_frustum.

                m_renderer.get_system<render_system_debug>()->add_frustum(cluster.bounds, color::purple);
            }
        }
    }

    // Run the clustering in parallel.
    parallel_for("cluster lights", task_queue::standard, k_total_clusters, [&lights, &clusters](size_t index) mutable {
    
        cluster& cluster = clusters.clusters[index];

        // Do bounds checks for each light.
        for (render_light* light : lights)
        {
            obb bounds = light->get_bounds();
            if (cluster.bounds.intersects(bounds) != frustum::intersection::outside)
            {
                cluster.lights.push_back(light);
            }
        }
    
    }, true);
}
*/

void render_system_lighting::build_graph(render_graph& graph, const render_world_state& state, render_view& view)
{
    if (!view.has_flag(render_view_flags::normal))
    {
        return;
    }

    render_system_shadows* shadow_system = m_renderer.get_system<render_system_shadows>();

    render_scene_manager& scene_manager = m_renderer.get_scene_manager();
    render_batch_instance_buffer* light_instance_buffer = view.get_resource_cache().find_or_create_instance_buffer(this);
    render_batch_instance_buffer* shadow_map_instance_buffer = view.get_resource_cache().find_or_create_instance_buffer(this + 1);
    ri_param_block* resolve_param_block = view.get_resource_cache().find_or_create_param_block(this, "resolve_lighting_parameters");

    //http://www.aortiz.me/2018/12/21/CG.html
    //todo

    // Grab all lights that can directly effect the frustum.
    // TODO: Doing an octtree query should be faster than this, reconsider.
    std::vector<render_light*> visible_lights;
    for (auto& light : scene_manager.get_directional_lights())
    {
        if (view.is_object_visible(light))
        {
            visible_lights.push_back(light);
        }
    }
    for (auto& light : scene_manager.get_point_lights())
    {
        if (view.is_object_visible(light))
        {
            visible_lights.push_back(light);
        }
    }
    for (auto& light : scene_manager.get_spot_lights())
    {
        if (view.is_object_visible(light))
        {
            visible_lights.push_back(light);
        }
    }
    
    // Cluster all lights into our volume.
    // TODO: This should all move to a compute shader.
    //cluster_list clusters;
    //cluster_lights(view, visible_lights, clusters);


    // Fill in the indirection buffers.
    int total_lights = 0;
    int total_shadow_maps = 0;

    {
        profile_marker(profile_colors::render, "Build light buffer");

        for (render_light* light : visible_lights)
        {
            ri_param_block* light_state_block = light->get_light_state_param_block();
            render_system_shadows::shadow_info& shadows = shadow_system->find_or_create_shadow_info(light->get_id(), view.get_id());

            // Make sure we have space left in the lists.
            if (total_lights + 1 >= k_max_lights ||
                total_shadow_maps + shadows.cascades.size() >= k_max_shadow_maps)
            {
                break;
            }

            // Skip light if beyond importance range.
            float distance = (view.get_local_location() - light->get_local_location()).length();
            if (distance > light->get_importance_distance())
            {
                continue;
            }

            // Add light instance to buffer.
            size_t index, offset;
            light_state_block->set("shadow_map_start_index", total_shadow_maps);
            light_state_block->set("shadow_map_count", (int)shadows.cascades.size());
            light_state_block->get_table(index, offset);
            light_instance_buffer->add(index, offset);

            // Add each shadow info to buffer.
            for (render_system_shadows::cascade_info& cascade : shadows.cascades)
            {
                cascade.shadow_map_state_param_block->get_table(index, offset);
                shadow_map_instance_buffer->add(index, offset);
            }

            total_lights++;
            total_shadow_maps += shadows.cascades.size();
        }

        light_instance_buffer->commit();
        shadow_map_instance_buffer->commit();
    }

    // Get the cluster states.
    vector3u grid_size;
    size_t cluster_size;
    size_t max_visible_lights;
    get_cluster_values(grid_size, cluster_size, max_visible_lights);

    // Update the number of lights we have in the buffer.
    {
        profile_marker(profile_colors::render, "Update resolve params");

        float z_near = 0.0f;
        float z_far = 0.0f;
        view.get_clip(z_near, z_far);

        resolve_param_block->set("light_count", total_lights);
        resolve_param_block->set("light_buffer", light_instance_buffer->get_buffer());
        resolve_param_block->set("shadow_map_count", total_shadow_maps);
        resolve_param_block->set("shadow_map_buffer", shadow_map_instance_buffer->get_buffer());
        resolve_param_block->set("shadow_map_sampler", *m_renderer.get_default_sampler(default_sampler_type::shadow_map));
        resolve_param_block->set("visualization_mode", (int)m_renderer.get_visualization_mode());
        resolve_param_block->set("light_grid_size", grid_size);
        resolve_param_block->set("light_cluster_buffer", *m_light_cluster_buffer, true);
        resolve_param_block->set("light_cluster_visibility_buffer", *m_light_cluster_visibility_buffer, true);
        resolve_param_block->set("light_cluster_visibility_count_buffer", *m_light_cluster_visibility_count_buffer, true);
    }

    // Add pass to run compute shader to generate the clusters.
    std::unique_ptr<render_pass_compute> cluster_pass = std::make_unique<render_pass_compute>();
    cluster_pass->name = "generate light clusters";
    cluster_pass->system = this;
    cluster_pass->technique = m_renderer.get_effect_manager().get_technique("create_light_clusters", {});
    cluster_pass->param_blocks.push_back(resolve_param_block);
    cluster_pass->param_blocks.push_back(view.get_view_info_param_block());
    graph.add_node(std::move(cluster_pass));

    // Add pass to run compute shader to cluster our lights.
    std::unique_ptr<render_pass_compute> cull_pass = std::make_unique<render_pass_compute>();
    cull_pass->name = "cull lights";
    cull_pass->system = this;
    cull_pass->technique = m_renderer.get_effect_manager().get_technique("cull_lights", {});
    cull_pass->param_blocks.push_back(resolve_param_block);
    cull_pass->param_blocks.push_back(view.get_view_info_param_block());
    graph.add_node(std::move(cull_pass));

    // Add pass to generate the light accumulation buffer.
    std::unique_ptr<render_pass_fullscreen> pass = std::make_unique<render_pass_fullscreen>();
    pass->name = "resolve lighting";
    pass->system = this;
    pass->technique = m_renderer.get_effect_manager().get_technique("resolve_lighting", {});
    pass->output = m_lighting_output;
    pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    pass->param_blocks.push_back(view.get_view_info_param_block());
    pass->param_blocks.push_back(resolve_param_block);
    graph.add_node(std::move(pass));
}

void render_system_lighting::get_cluster_values(vector3u& out_grid_size, size_t& out_cluster_size, size_t& out_max_lights_per_cluster)
{
    // Grab some information from the culling light.
    render_effect::technique* cull_lights_technique = m_renderer.get_effect_manager().get_technique("cull_lights", {});

    // Grab light grid size.
    size_t light_grid_size_x;
    size_t light_grid_size_y;
    size_t light_grid_size_z;

    if (!cull_lights_technique->get_define<size_t>("LIGHT_GRID_SIZE_X", light_grid_size_x) ||
        !cull_lights_technique->get_define<size_t>("LIGHT_GRID_SIZE_Y", light_grid_size_y) ||
        !cull_lights_technique->get_define<size_t>("LIGHT_GRID_SIZE_Z", light_grid_size_z) ||
        !cull_lights_technique->get_define<size_t>("MAX_LIGHTS_PER_CLUSTER", out_max_lights_per_cluster))
    {
        db_fatal(renderer, "Failed to get light grid size from cull_lights technique.");
        return;
    }

    out_grid_size = vector3u(light_grid_size_x, light_grid_size_y, light_grid_size_z);
    out_cluster_size = m_renderer.get_param_block_manager().get_param_block_archetype("light_cluster")->get_size();
}

void render_system_lighting::step(const render_world_state& state)
{
    vector3u grid_size;
    size_t cluster_size;
    size_t max_lights_per_cluster;
    get_cluster_values(grid_size, cluster_size, max_lights_per_cluster);

    size_t cluster_count = grid_size.x * grid_size.y * grid_size.z;
    size_t max_visible_lights = cluster_count * max_lights_per_cluster;

    // Make sure cluster buffer exists, or recreate it if the grid size information has changed.
    if (!m_light_cluster_buffer || 
         m_light_cluster_buffer->get_element_count() < cluster_count || 
         m_light_cluster_buffer->get_element_size() != cluster_size)
    {
        // Create a buffer which will store general information about each light cluster, such as its aabb.
        ri_buffer::create_params buffer_params;
        buffer_params.element_count = cluster_count;
        buffer_params.element_size = cluster_size;
        buffer_params.usage = ri_buffer_usage::generic;
        m_light_cluster_buffer = m_renderer.get_render_interface().create_buffer(buffer_params, "light clusters");
    }

    // Make sure the visibility buffer is valid.
    // This buffer stores lists of which lights are visible to which clusters.
    if (!m_light_cluster_visibility_buffer ||
         m_light_cluster_visibility_buffer->get_element_count() != max_visible_lights)
    {
        ri_buffer::create_params buffer_params;
        buffer_params.element_count = max_visible_lights;
        buffer_params.element_size = 4;
        buffer_params.usage = ri_buffer_usage::generic;
        m_light_cluster_visibility_buffer = m_renderer.get_render_interface().create_buffer(buffer_params, "light cluster visibility");
    }

    // Make sure the visibility count buffer is valid.
    // This buffer is ridiculously simple. Its just enough space to store an int that atomically
    // increments as items are added to the visibility buffer.
    if (!m_light_cluster_visibility_count_buffer)
    {
        ri_buffer::create_params buffer_params;
        buffer_params.element_count = 1;
        buffer_params.element_size = 4;
        buffer_params.usage = ri_buffer_usage::generic;
        m_light_cluster_visibility_count_buffer = m_renderer.get_render_interface().create_buffer(buffer_params, "light cluster visibility count");
    }
}

}; // namespace ws
