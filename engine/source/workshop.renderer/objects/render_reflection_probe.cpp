// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/objects/render_reflection_probe.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.renderer/systems/render_system_lighting.h"
#include "workshop.renderer/systems/render_system_reflection_probes.h"
#include "workshop.render_interface/ri_interface.h"

namespace ws {

render_reflection_probe::render_reflection_probe(render_object_id id, renderer& in_renderer)
    : render_object(id, &in_renderer, render_visibility_flags::physical)
{
    m_param_block = m_renderer->get_param_block_manager().create_param_block("reflection_probe_state");

    ri_texture::create_params params;
    params.width = render_system_reflection_probes::k_probe_cubemap_size;
    params.height = render_system_reflection_probes::k_probe_cubemap_size;
    params.depth = 6;
    params.mip_levels = render_system_reflection_probes::k_probe_cubemap_mips;
    params.dimensions = ri_texture_dimension::texture_cube;
    params.is_render_target = true;
    params.format = ri_texture_format::R16G16B16A16_FLOAT;
    m_texture = m_renderer->get_render_interface().create_texture(params, "reflection probe");
}

render_reflection_probe::~render_reflection_probe()
{
}

obb render_reflection_probe::get_bounds()
{
    return obb(
        aabb(vector3(-0.5f, -0.5f, -0.5f), vector3(0.5f, 0.5f, 0.5f)), 
        get_transform()
    );
}

void render_reflection_probe::bounds_modified()
{
    render_object::bounds_modified();

    m_dirty = true;

    m_param_block->set("probe_texture", *m_texture);
    m_param_block->set("probe_texture_sampler", *m_renderer->get_default_sampler(default_sampler_type::color));    
    m_param_block->set("world_position", get_local_location());
    m_param_block->set("radius", get_local_scale().max_component() * 0.5f);
    m_param_block->set("mip_levels", (int)m_texture->get_mip_levels());
}

ri_texture& render_reflection_probe::get_texture()
{
    return *m_texture;
}

ri_param_block& render_reflection_probe::get_param_block()
{
    return *m_param_block;
}

bool render_reflection_probe::is_dirty()
{
    return m_dirty;
}

bool render_reflection_probe::is_ready()
{
    return m_ready;
}

void render_reflection_probe::mark_regenerated()
{
    m_dirty = false;
    m_ready = true;
}

}; // namespace ws
