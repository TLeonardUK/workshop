// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/objects/render_directional_light.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.renderer/systems/render_system_lighting.h"
#include "workshop.render_interface/ri_interface.h"

namespace ws {

render_directional_light::render_directional_light(render_object_id id, renderer& renderer)
    : render_light(id, renderer, render_visibility_flags::physical)
{
    // Default to a really high importance distance for directional lights as we typically do not
    // want them fading out.
    m_importance_distance = 100000000.0f;
}

render_directional_light::~render_directional_light()
{
}

void render_directional_light::set_shadow_cascades(size_t value)
{
    m_shadow_map_cascades = value;
}

size_t render_directional_light::get_shodow_cascades()
{
    return m_shadow_map_cascades;
}

void render_directional_light::set_shadow_cascade_exponent(float value)
{
    m_shadow_map_exponent = value;
}

float render_directional_light::get_shodow_cascade_exponent()
{
    return m_shadow_map_exponent;
}

void render_directional_light::set_shadow_cascade_blend(float value)
{
    m_shadow_map_blend = value;

    update_render_data();
}

float render_directional_light::get_shodow_cascade_blend()
{
    return m_shadow_map_blend;
}

void render_directional_light::update_render_data()
{
    render_light::update_render_data();

    m_light_state_param_block->set("type", (int)render_light_type::directional);
    m_light_state_param_block->set("inner_radius", 0.0f);
    m_light_state_param_block->set("outer_radius", 0.0f);
    m_light_state_param_block->set("cascade_blend_factor", get_shodow_cascade_blend());
}

obb render_directional_light::get_bounds()
{
    aabb bounds(
        vector3(-10000000.0f, -10000000.0f, -10000000.0f),
        vector3( 10000000.0f,  10000000.0f,  10000000.0f)
    );

    return obb(bounds, get_transform());
}

}; // namespace ws
