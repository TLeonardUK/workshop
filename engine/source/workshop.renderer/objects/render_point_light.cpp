// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/objects/render_point_light.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.renderer/systems/render_system_lighting.h"
#include "workshop.renderer/systems/render_system_debug.h"

namespace ws {

render_point_light::render_point_light(render_object_id id, renderer& renderer)
    : render_light(id, renderer, render_visibility_flags::physical)
{
}

render_point_light::~render_point_light()
{
}

void render_point_light::update_render_data()
{
    render_light::update_render_data();

    m_light_state_param_block->set("type", (int)render_light_type::point);
    m_light_state_param_block->set("inner_radius", 0.0f);
    m_light_state_param_block->set("outer_radius", 0.0f);
    m_light_state_param_block->set("cascade_blend_factor", 0.0f);
}

obb render_point_light::get_bounds()
{
    aabb bounds(
        vector3(-m_range, -m_range, -m_range),
        vector3(m_range, m_range, m_range)    
    );

    return obb(bounds, get_transform());
}

}; // namespace ws
