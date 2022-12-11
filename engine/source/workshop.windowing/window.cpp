// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.windowing/window.h"

namespace ws {

void window::set_title(const std::string& title)
{
    m_title = title;
    m_title_dirty = true;
}

std::string window::get_title()
{
    return m_title;
}

void window::set_width(size_t value)
{
    m_width = value;
    m_size_dirty = true;
}

size_t window::get_width()
{
    return m_width;
}

void window::set_height(size_t value)
{
    m_height = value;
    m_size_dirty = true;
}

size_t window::get_height()
{
    return m_height;
}

void window::set_mode(window_mode mode)
{
    m_mode = mode;
    m_mode_dirty = true;
}

window_mode window::get_mode()
{
    return m_mode;
}

ri_interface_type window::get_compatibility()
{
    return m_compatibility;
}

void window::set_compatibility(ri_interface_type value)
{
    m_compatibility = value;
}

}; // namespace ws
