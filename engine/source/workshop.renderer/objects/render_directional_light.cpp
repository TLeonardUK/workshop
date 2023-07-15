// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/objects/render_directional_light.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.renderer/systems/render_system_lighting.h"

namespace ws {

render_directional_light::render_directional_light(render_scene_manager* scene_manager, renderer& renderer)
    : render_object(scene_manager)
    , m_renderer(renderer)
{
    m_light_state_param_block = m_renderer.get_param_block_manager().create_param_block("light_state");
}

render_directional_light::~render_directional_light()
{
}

void render_directional_light::set_shadow_casting(bool value)
{
    m_shadow_casting = value;
}

bool render_directional_light::get_shodow_casting()
{
    return m_shadow_casting;
}

void render_directional_light::set_shadow_map_size(size_t value)
{
    m_shadow_map_size = value;
}

size_t render_directional_light::get_shodow_map_size()
{
    return m_shadow_map_size;
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

void render_directional_light::set_shadow_max_distance(float value)
{
    m_shadow_max_distance = value;
}

float render_directional_light::get_shodow_max_distance()
{
    return m_shadow_max_distance;
}

void render_directional_light::set_shadow_cascade_blend(float value)
{
    m_shadow_cascade_blend = value;
}

float render_directional_light::get_shodow_cascade_blend()
{
    return m_shadow_cascade_blend;
}

void render_directional_light::set_local_transform(const vector3& location, const quat& rotation, const vector3& scale)
{
    render_object::set_local_transform(location, rotation, scale);

    update_render_data();
}

void render_directional_light::update_render_data()
{
    vector3 world_location = m_local_location;
    vector3 world_direction = vector3::forward * m_local_rotation;

    m_light_state_param_block->set("type", (int)render_light_type::directional);
    m_light_state_param_block->set("position", world_location);
    m_light_state_param_block->set("direction", world_direction);
    m_light_state_param_block->set("color", vector3(1.0f, 1.0f, 1.0f));
}

ri_param_block* render_directional_light::get_light_state_param_block()
{
    return m_light_state_param_block.get();
}

obb render_directional_light::get_bounds()
{
    aabb bounds(
        vector3(-100000.0f, -100000.0f, -100000.0f),
        vector3( 100000.0f,  100000.0f,  100000.0f)
    );

    return obb(bounds, get_transform());
}

}; // namespace ws
