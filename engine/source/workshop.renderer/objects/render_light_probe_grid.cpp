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

namespace ws {

render_light_probe_grid::render_light_probe_grid(render_object_id id, renderer& in_renderer)
    : render_object(id, &in_renderer, render_visibility_flags::physical)
{
    m_param_block = m_renderer->get_param_block_manager().create_param_block("light_probe_grid_state");

    render_effect::technique* main_technique = m_renderer->get_effect_manager().get_technique("raytrace_diffuse_probe_output_irradiance", {});
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

    size_t occlusion_required_space = m_occlusion_map_size + map_padding;
    size_t occlusion_texture_size = math::calculate_atlas_size(occlusion_required_space, m_probes.size());

    ri_texture::create_params texture_params;
    texture_params.allow_unordered_access = true;
    texture_params.width = occlusion_texture_size;
    texture_params.height = occlusion_texture_size;
    texture_params.format = ri_texture_format::R32G32_FLOAT;
    m_occlusion_texture = m_renderer->get_render_interface().create_texture(texture_params, "light grid occlusion");

    size_t irradiance_required_space = m_irradiance_map_size + map_padding;
    size_t irradiance_texture_size = math::calculate_atlas_size(irradiance_required_space, m_probes.size());

    texture_params.width = irradiance_texture_size;
    texture_params.height = irradiance_texture_size;
    texture_params.format = ri_texture_format::R11G11B10_FLOAT;
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

    // Update individual probes.
    for (size_t z = 0; z < m_depth; z++)
    {
        for (size_t y = 0; y < m_height; y++)
        {
            for (size_t x = 0; x < m_width; x++)
            {
                size_t probe_index = get_probe_index(x, y, z);

                probe& probe = m_probes[probe_index];
                probe.dirty = true;
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

                probe.debug_param_block = m_renderer->get_param_block_manager().create_param_block("light_probe_instance_info");
                probe.debug_param_block->set("model_matrix", matrix4::scale(vector3(25.0f, 25.0f, 25.0f)) * matrix4::translate(probe.origin));

                size_t table_index, table_offset;
                m_param_block->get_table(table_index, table_offset);

                probe.debug_param_block->set("grid_state_table_index", (int)table_index);
                probe.debug_param_block->set("grid_state_table_offset", (int)table_offset);

                probe.debug_param_block->set("grid_coord", vector3i((int)x, (int)y, (int)z));
            }
        }
    }

}

}; // namespace ws
