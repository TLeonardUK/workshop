// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/objects/render_light_probe_grid.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.renderer/systems/render_system_lighting.h"
#include "workshop.render_interface/ri_interface.h"

namespace ws {

render_light_probe_grid::render_light_probe_grid(render_object_id id, render_scene_manager* scene_manager, renderer& in_renderer)
    : render_object(id, scene_manager)
    , m_renderer(in_renderer)
{
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

ri_buffer& render_light_probe_grid::get_spherical_harmonic_buffer()
{
    return *m_spherical_harmonic_buffer;
}

void render_light_probe_grid::recalculate_probes()
{
    vector3 bounds = get_local_scale();

    m_width = static_cast<size_t>(floor(bounds.x / m_density));
    m_height = static_cast<size_t>(floor(bounds.y / m_density));
    m_depth = static_cast<size_t>(floor(bounds.z / m_density));

    m_probes.resize(m_width * m_height * m_depth);

    matrix4 grid_transform =
        matrix4::rotation(m_local_rotation) *
        matrix4::translate(m_local_location);

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
                    (-bounds.x * 0.5f) + ((x * m_density) + (m_density * 0.5f)),
                    (-bounds.y * 0.5f) + ((y * m_density) + (m_density * 0.5f)),
                    (-bounds.z * 0.5f) + ((z * m_density) + (m_density * 0.5f))
                ) * grid_transform;
                probe.extents = vector3(
                    m_density * 0.5f,
                    m_density * 0.5f,
                    m_density * 0.5f
                );
                probe.orientation = m_local_rotation;
            }
        }
    }

    // Create the buffer that will hold our SH values.
    ri_buffer::create_params buffer_params;
    buffer_params.element_count = std::max(1llu, m_probes.size());
    buffer_params.element_size = k_probe_coefficient_size;
    buffer_params.usage = ri_buffer_usage::generic;
    m_spherical_harmonic_buffer = m_renderer.get_render_interface().create_buffer(buffer_params, "light grid sh coefficients");
}

}; // namespace ws
