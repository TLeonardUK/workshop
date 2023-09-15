// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/objects/render_spot_light.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.renderer/systems/render_system_lighting.h"
#include "workshop.renderer/systems/render_system_debug.h"

namespace ws {

render_spot_light::render_spot_light(render_object_id id, renderer& renderer)
    : render_light(id, renderer, render_visibility_flags::physical)
{
}

render_spot_light::~render_spot_light()
{
}

void render_spot_light::set_radius(float inner, float outer)
{
    m_inner_radius = inner;
    m_outer_radius = outer;

    update_render_data();
}

void render_spot_light::get_radius(float& inner, float& outer)
{
    inner = m_inner_radius;
    outer = m_outer_radius;
}

void render_spot_light::update_render_data()
{
    render_light::update_render_data();

    m_light_state_param_block->set("type", (int)render_light_type::spotlight);
    m_light_state_param_block->set("inner_radius", m_inner_radius);
    m_light_state_param_block->set("outer_radius", m_outer_radius);
    m_light_state_param_block->set("cascade_blend_factor", 0.0f);
}

obb render_spot_light::get_bounds()
{
    // This is basically treading the spot light as though
    // its a point light.
    // TODO We can cull this better.
    aabb bounds(
        vector3(-m_range, -m_range, -m_range),
        vector3(m_range, m_range, m_range)
    );

    return obb(bounds, get_transform());
}

void render_spot_light::debug_draw(render_system_debug& debug)
{
    if (has_render_gpu_flag(render_gpu_flags::selected))
    {
        vector3 world_location = m_local_location;
        vector3 world_direction = vector3::forward * m_local_rotation;

        vector3 world_location_end = world_location + (world_direction * m_range);

        vector3 world_direction_outer = (vector3::forward * quat::angle_axis(m_outer_radius * 2.0f, vector3::up)) * m_local_rotation;
        vector3 world_location_end_outer = world_location + (world_direction_outer * m_range);

        vector3 world_direction_inner = (vector3::forward * quat::angle_axis(m_inner_radius * 2.0f, vector3::up)) * m_local_rotation;
        vector3 world_location_end_inner = world_location + (world_direction_inner * m_range);

        float outer_radius = (world_location_end - world_location_end_outer).length();
        float inner_radius = (world_location_end - world_location_end_inner).length();

        debug.add_cone(world_location_end, world_location, outer_radius, color::white);
        debug.add_cone(world_location_end, world_location, inner_radius, color::white);
    }
}

}; // namespace ws
