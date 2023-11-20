// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/objects/render_light_probe_grid.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.renderer/systems/render_system_lighting.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.core/memory/memory_tracker.h"

namespace ws {

render_light_probe_grid::render_light_probe_grid(render_object_id id, renderer& in_renderer)
    : render_object(id, &in_renderer, render_visibility_flags::physical)
{
    memory_scope scope(memory_type::rendering__light_probe_grid, memory_scope::k_ignore_asset);

    m_param_block = m_renderer->get_param_block_manager().create_param_block("light_probe_grid_state");

    render_effect::technique* main_technique = m_renderer->get_effect_manager().get_technique("ddgi_output_irradiance", {});
    if (!main_technique->get_define<size_t>("PROBE_GRID_IRRADIANCE_MAP_SIZE", m_irradiance_map_size) ||
        !main_technique->get_define<size_t>("PROBE_GRID_OCCLUSION_MAP_SIZE", m_occlusion_map_size))
    {   
        db_fatal(renderer, "Failed to retrieve expected defines from light probe shaders.");
        return;
    }
}

render_light_probe_grid::~render_light_probe_grid()
{
}

void render_light_probe_grid::set_density(float value)
{
    m_density = value;

    bounds_modified();
}

float render_light_probe_grid::get_density()
{
    return m_density;
}

obb render_light_probe_grid::get_bounds()
{
    return obb(
        aabb(vector3(-0.5f, -0.5f, -0.5f), vector3(0.5f, 0.5f, 0.5f)), 
        get_transform()
    );
}

std::vector<render_light_probe_grid::probe>& render_light_probe_grid::get_probes()
{
    return m_probes;
}

void render_light_probe_grid::bounds_modified()
{
    render_object::bounds_modified();

    recalculate_probes();
}

size_t render_light_probe_grid::get_probe_index(size_t x, size_t y, size_t z)
{
    return 
        ((m_width * m_height) * z) +
        (m_width * y) +
        x;
}

ri_buffer& render_light_probe_grid::get_probe_state_buffer()
{
    return *m_probe_state_buffer;
}

ri_texture& render_light_probe_grid::get_occlusion_texture()
{
    return *m_occlusion_texture;
}

ri_texture& render_light_probe_grid::get_irradiance_texture()
{
    return *m_irradiance_texture;
}

ri_param_block& render_light_probe_grid::get_param_block()
{
    return *m_param_block;
}

size_t render_light_probe_grid::get_irradiance_map_size()
{
    return m_irradiance_map_size;
}

size_t render_light_probe_grid::get_occlusion_map_size()
{
    return m_occlusion_map_size;
}

void render_light_probe_grid::recalculate_probes()
{
    memory_scope scope(memory_type::rendering__light_probe_grid, memory_scope::k_ignore_asset);

    const render_options& options = m_renderer->get_options();

    vector3 bounds = get_local_scale();

    m_width = static_cast<size_t>(floor(bounds.x / m_density));
    m_height = static_cast<size_t>(floor(bounds.y / m_density));
    m_depth = static_cast<size_t>(floor(bounds.z / m_density));

    m_width = std::min(m_width, k_max_dimension);
    m_height = std::min(m_height, k_max_dimension);
    m_depth = std::min(m_depth, k_max_dimension);

    m_probes.resize(m_width * m_height * m_depth);

    size_t map_padding = 2;

    // Create buffer to store the transient gpu state of each probe.
    ri_param_block_archetype* state_archetype = m_renderer->get_param_block_manager().get_param_block_archetype("light_probe_state");

    ri_buffer::create_params buffer_params;
    buffer_params.element_count = std::max(1llu, m_probes.size());
    buffer_params.element_size = state_archetype->get_size();
    buffer_params.usage = ri_buffer_usage::generic;
    m_probe_state_buffer = m_renderer->get_render_interface().create_buffer(buffer_params, "light grid probe state buffer");

    // Create atlas to store occlusion data for each probe in.
    size_t occlusion_required_space = m_occlusion_map_size + map_padding;
    size_t occlusion_texture_size = math::calculate_atlas_size(occlusion_required_space, m_probes.size());

    ri_texture::create_params texture_params;
    texture_params.allow_unordered_access = true;
    texture_params.width = occlusion_texture_size;
    texture_params.height = occlusion_texture_size;
    texture_params.format = ri_texture_format::R32G32_FLOAT;
    m_occlusion_texture = m_renderer->get_render_interface().create_texture(texture_params, "light grid occlusion");

    // Create atlas to store irradiance data for each probe in.
    size_t irradiance_required_space = m_irradiance_map_size + map_padding;
    size_t irradiance_texture_size = math::calculate_atlas_size(irradiance_required_space, m_probes.size());

    texture_params.width = irradiance_texture_size;
    texture_params.height = irradiance_texture_size;
    texture_params.format = ri_texture_format::R32G32B32A32_FLOAT;//R11G11B10_FLOAT;
    m_irradiance_texture = m_renderer->get_render_interface().create_texture(texture_params, "light grid irradiance");

    matrix4 grid_transform =
        matrix4::rotation(m_local_rotation) *
        matrix4::translate(m_local_location);

    // Update the param block description.
    m_param_block->set("world_to_grid_matrix", grid_transform.inverse());
    m_param_block->set("grid_to_world_matrix", grid_transform);
    m_param_block->set("size", vector3i((int)m_width, (int)m_height, (int)m_depth));
    m_param_block->set("bounds", bounds);
    m_param_block->set("density", m_density);
    m_param_block->set("irradiance_texture", *m_irradiance_texture);
    m_param_block->set("occlusion_texture", *m_occlusion_texture);
    m_param_block->set("map_sampler", *m_renderer->get_default_sampler(default_sampler_type::bilinear));
    m_param_block->set("irradiance_texture_size", (int)m_irradiance_texture->get_width());
    m_param_block->set("irradiance_map_size", (int)m_irradiance_map_size);
    m_param_block->set("irradiance_probes_per_row", (int)(m_irradiance_texture->get_width() / irradiance_required_space));
    m_param_block->set("occlusion_texture_size", (int)m_occlusion_texture->get_width());
    m_param_block->set("occlusion_map_size", (int)m_occlusion_map_size);
    m_param_block->set("occlusion_probes_per_row", (int)(m_occlusion_texture->get_width() / occlusion_required_space));
    m_param_block->set("view_bias", options.light_probe_view_bias);
    m_param_block->set("normal_bias", options.light_probe_normal_bias);
    m_param_block->set("probe_state_buffer", *m_probe_state_buffer, true);

    // Update individual probes.
    for (size_t z = 0; z < m_depth; z++)
    {
        for (size_t y = 0; y < m_height; y++)
        {
            for (size_t x = 0; x < m_width; x++)
            {
                size_t probe_index = get_probe_index(x, y, z);

                probe& probe = m_probes[probe_index];
                probe.index = probe_index;
                probe.origin = vector3(
                    (-bounds.x * 0.5f) + ((x * m_density)),
                    (-bounds.y * 0.5f) + ((y * m_density)),
                    (-bounds.z * 0.5f) + ((z * m_density))
                ) * grid_transform;
                probe.extents = vector3(
                    m_density * 0.5f,
                    m_density * 0.5f,
                    m_density * 0.5f
                );
                probe.orientation = m_local_rotation;

                // Make "normal" param block
                // TODO: Get rid of this some how, we end up spending more memory on this than the maps!
                probe.param_block = m_renderer->get_param_block_manager().create_param_block("ddgi_probe_data");
                probe.param_block->set("probe_origin", probe.origin);
                probe.param_block->set("probe_index", (int)probe.index);
                probe.param_block->set("irradiance_texture", ri_texture_view(&get_irradiance_texture(), 0), true);
                probe.param_block->set("irradiance_map_size", (int)get_irradiance_map_size());
                probe.param_block->set("irradiance_per_row", (int)(get_irradiance_texture().get_width() / (get_irradiance_map_size() + map_padding)));
                probe.param_block->set("occlusion_texture", ri_texture_view(&get_occlusion_texture(), 0), true);
                probe.param_block->set("occlusion_map_size", (int)get_occlusion_map_size());
                probe.param_block->set("occlusion_per_row", (int)(get_occlusion_texture().get_width() / (get_occlusion_map_size() + map_padding)));
                probe.param_block->set("probe_spacing", get_density());
                probe.param_block->set("probe_state_buffer", get_probe_state_buffer(), true);

                // Make debug param block.
                probe.debug_param_block = m_renderer->get_param_block_manager().create_param_block("light_probe_instance_info");
                probe.debug_param_block->set("model_matrix", matrix4::scale(vector3(25.0f, 25.0f, 25.0f)) * matrix4::translate(probe.origin));
                probe.debug_param_block->set("scale", vector3(25.0f, 25.0f, 25.0f));

                size_t table_index, table_offset;
                m_param_block->get_table(table_index, table_offset);

                probe.debug_param_block->set("grid_state_table_index", (int)table_index);
                probe.debug_param_block->set("grid_state_table_offset", (int)table_offset);

                probe.debug_param_block->set("grid_coord", vector3i((int)x, (int)y, (int)z));
            }
        }
    }
}

void render_light_probe_grid::get_probes_to_update(std::vector<frustum>& frustums, std::vector<size_t>& onscreen_probe_indices, std::vector<size_t>& offscreen_probe_indices)
{
    memory_scope scope(memory_type::rendering__light_probe_grid, memory_scope::k_ignore_asset);

    vector3 local_bounds = get_local_scale();
    matrix4 grid_transform =
        matrix4::rotation(m_local_rotation) *
        matrix4::translate(m_local_location);

    std::function<void(int, int, int, int, int, int)> check_area;

    // How small an axis has to get before considering that we've gotten to the leaf quadrants.
    constexpr int leaf_axis_threshold = 4;

    // Iterate through grid breaking into into quadrants and see which are in frustum, this allows us to break updates
    // down into small blocks of spatially relative blocks of probes, which looks more natural to update at once.
    check_area = [this, &grid_transform, &local_bounds, &frustums, &check_area, leaf_axis_threshold, &onscreen_probe_indices, &offscreen_probe_indices](int x, int y, int z, int width, int height, int depth) mutable {

        vector3 area_min = vector3(
            (-local_bounds.x * 0.5f) + (x * m_density),
            (-local_bounds.y * 0.5f) + (y * m_density),
            (-local_bounds.z * 0.5f) + (z * m_density)
        );
        vector3 area_max = vector3(
            (-local_bounds.x * 0.5f) + ((x + width) * m_density),
            (-local_bounds.y * 0.5f) + ((y + height) * m_density),
            (-local_bounds.z * 0.5f) + ((z + depth) * m_density)
        );

        aabb area_local_aabb = aabb(area_min, area_max);
        obb area_world_bounds(area_local_aabb, grid_transform);
        
        bool visible = false;

        // Check if area is visible in any of the frustrums
        for (frustum& f : frustums)
        {
            if (f.intersects(area_world_bounds) != frustum::intersection::outside)
            {
                visible = true;
                break;
            }
        }

        // Can't break down any more, take all probes in this area.
        if (width <= leaf_axis_threshold || height <= leaf_axis_threshold || depth <= leaf_axis_threshold)
        {
            for (int px = 0; px < width; px++)
            {
                for (int py = 0; py < height; py++)
                {
                    for (int pz = 0; pz < depth; pz++)
                    {
                        size_t probe_index = get_probe_index(x + px, y + py, z + pz);

                        if (visible)
                        {
                            onscreen_probe_indices.push_back(probe_index);
                        }
                        else
                        {
                            offscreen_probe_indices.push_back(probe_index);
                        }
                    }
                }
            }
        }
        // Break into octants and check them.
        else
        {
            int half_width_ceil = (int)std::ceilf(width * 0.5f);
            int half_height_ceil = (int)std::ceilf(height * 0.5f);
            int half_depth_ceil = (int)std::ceilf(depth * 0.5f);
            int half_width_floor = (int)std::floorf(width * 0.5f);
            int half_height_floor = (int)std::floorf(height * 0.5f);
            int half_depth_floor = (int)std::floorf(depth * 0.5f);

            check_area(x, y, z, half_width_floor, half_height_floor, half_depth_floor);
            check_area(x + half_width_floor, y, z, half_width_ceil, half_height_floor, half_depth_floor);
            check_area(x, y + half_height_floor, z, half_width_floor, half_height_ceil, half_depth_floor);
            check_area(x + half_width_floor, y + half_height_floor, z, half_width_ceil, half_height_ceil, half_depth_floor);

            check_area(x, y, z + half_depth_floor, half_width_floor, half_height_floor, half_depth_ceil);
            check_area(x + half_width_floor, y, z + half_depth_floor, half_width_ceil, half_height_floor, half_depth_ceil);
            check_area(x, y + half_height_floor, z + half_depth_floor, half_width_floor, half_height_ceil, half_depth_ceil);
            check_area(x + half_width_floor, y + half_height_floor, z + half_depth_floor, half_width_ceil, half_height_ceil, half_depth_ceil);
        }

    };

    // Start by checking the full area.
    check_area(0, 0, 0, (int)m_width, (int)m_height, (int)m_depth);

    //db_log(core, "visible_probes:%zi / %zi", out_probe_indices.size(), m_width * m_height * m_depth);
}

}; // namespace ws
