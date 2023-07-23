// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_object.h"
#include "workshop.renderer/render_scene_manager.h"

namespace ws {

render_object::render_object(render_object_id id, render_scene_manager* scene_manager, bool stored_in_octtree)
    : m_id(id)
    , m_scene_manager(scene_manager)
    , m_store_in_octtree(stored_in_octtree)
{
}

render_object::~render_object()
{
    if (m_store_in_octtree)
    {
        m_scene_manager->render_object_destroyed(this);
    }
}

void render_object::init()
{
    if (m_store_in_octtree)
    {
        m_scene_manager->render_object_created(this);
    }
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

obb render_object::get_bounds()
{
    return obb(aabb::zero, get_transform());
}

void render_object::bounds_modified()
{
    if (m_store_in_octtree)
    {
        m_scene_manager->render_object_bounds_modified(this);
    }
}

}; // namespace ws
