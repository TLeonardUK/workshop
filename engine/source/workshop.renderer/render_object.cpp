// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_object.h"
#include "workshop.renderer/render_visibility_manager.h"
#include "workshop.renderer/renderer.h"

namespace ws {

render_object::render_object(render_object_id id, renderer* renderer, render_visibility_flags visibility_flags)
    : m_id(id)
    , m_renderer(renderer)
    , m_visibility_flags(visibility_flags)
{
    m_visibility_id = m_renderer->get_visibility_manager().register_object(get_bounds(), m_visibility_flags);
}

render_object::~render_object()
{
    m_renderer->get_visibility_manager().unregister_object(m_visibility_id);
}

void render_object::init()
{
}

void render_object::set_name(const std::string& name)
{
    m_name = name;
}

std::string render_object::get_name()
{
    return m_name;
}

render_object_id render_object::get_id()
{
    return m_id;
}

void render_object::set_render_gpu_flags(render_gpu_flags flags)
{
    m_gpu_flags = flags;
}

render_gpu_flags render_object::get_render_gpu_flags()
{
    return m_gpu_flags;
}

bool render_object::has_render_gpu_flag(render_gpu_flags flag)
{
    return (m_gpu_flags & flag) != render_gpu_flags::none;
}

void render_object::set_visibility(bool flags)
{
    m_visibility = flags;
    m_renderer->get_visibility_manager().set_object_manual_visibility(m_visibility_id, flags);
}

bool render_object::get_visibility()
{
    return m_visibility;
}

void render_object::set_local_transform(const vector3& location, const quat& rotation, const vector3& scale)
{
    if (location == m_local_location &&
        rotation == m_local_rotation &&
        scale == m_local_scale)
    {
        return;
    }

    m_local_location = location;
    m_local_rotation = rotation;
    m_local_scale = scale;

    bounds_modified();
}

vector3 render_object::get_local_location()
{
    return m_local_location;
}

vector3 render_object::get_local_scale()
{
    return m_local_scale;
}

quat render_object::get_local_rotation()
{
    return m_local_rotation;
}

matrix4 render_object::get_transform()
{
    return matrix4::scale(m_local_scale) *
           matrix4::rotation(m_local_rotation) *
           matrix4::translate(m_local_location);
}

render_visibility_manager::object_id render_object::get_visibility_id()
{
    return m_visibility_id;
}

obb render_object::get_bounds()
{
    return obb(aabb::zero, get_transform());
}

void render_object::bounds_modified()
{
    m_renderer->get_visibility_manager().update_object_bounds(m_visibility_id, get_bounds());
}

}; // namespace ws
