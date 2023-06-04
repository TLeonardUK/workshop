// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_object.h"

namespace ws {

void render_object::set_name(const std::string& name)
{
    m_name = name;
}

std::string render_object::get_name()
{
    return m_name;
}

void render_object::set_local_transform(const vector3& location, const quat& rotation, const vector3& scale)
{
    m_local_location = location;
    m_local_rotation = rotation;
    m_local_scale = scale;
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

}; // namespace ws
