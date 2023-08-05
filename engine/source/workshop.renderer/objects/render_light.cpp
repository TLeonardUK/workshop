// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/objects/render_light.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.renderer/systems/render_system_lighting.h"

namespace ws {

render_light::render_light(render_object_id id, renderer& renderer, render_visibility_flags visibility_flags)
    : render_object(id, &renderer, visibility_flags)
{
    m_light_state_param_block = m_renderer->get_param_block_manager().create_param_block("light_state");
}

render_light::~render_light()
{
}

void render_light::set_color(color value)
{
    m_color = value;
}

color render_light::get_color()
{
    return m_color;
}

void render_light::set_intensity(float value)
{
    m_intensity = value;
}

float render_light::get_intensity()
{
    return m_intensity;
}

void render_light::set_range(float value)
{
    m_range = value;

    bounds_modified();
}

float render_light::get_range()
{
    return m_range;
}

void render_light::set_importance_distance(float value)
{
    m_importance_distance = value;
}

float render_light::get_importance_distance()
{
    return m_importance_distance;
}

void render_light::set_shadow_casting(bool value)
{
    m_shadow_casting = value;
}

bool render_light::get_shodow_casting()
{
    return m_shadow_casting;
}

void render_light::set_shadow_map_size(size_t value)
{
    m_shadow_map_size = value;
}

size_t render_light::get_shadow_map_size()
{
    return m_shadow_map_size;
}

void render_light::set_shadow_max_distance(float value)
{
    m_shadow_max_distance = value;
}

float render_light::get_shadow_max_distance()
{
    return m_shadow_max_distance;
}

void render_light::set_local_transform(const vector3& location, const quat& rotation, const vector3& scale)
{
    render_object::set_local_transform(location, rotation, scale);

    update_render_data();
}

void render_light::update_render_data()
{
    vector3 world_location = m_local_location;
    vector3 world_direction = vector3::forward * m_local_rotation;

    m_light_state_param_block->set("position", world_location);
    m_light_state_param_block->set("direction", world_direction);
    m_light_state_param_block->set("color", m_color.rgb());
    m_light_state_param_block->set("intensity", m_intensity);
    m_light_state_param_block->set("range", m_range);
    m_light_state_param_block->set("importance_distance", m_importance_distance);    
}

ri_param_block* render_light::get_light_state_param_block()
{
    return m_light_state_param_block.get();
}

}; // namespace ws
