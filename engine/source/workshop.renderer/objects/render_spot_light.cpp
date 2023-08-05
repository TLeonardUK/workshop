// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/objects/render_spot_light.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.renderer/systems/render_system_lighting.h"

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

}; // namespace ws
