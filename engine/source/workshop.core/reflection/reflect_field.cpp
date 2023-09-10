// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/reflection/reflect_field.h"

#include <algorithm>

namespace ws {

reflect_field::reflect_field(
        const char* name, 
        size_t offset, 
        size_t element_size, 
        std::type_index type, 
        std::type_index super_type, 
        const char* display_name, 
        const char* description, 
        reflect_field_container_type container_type,
        std::unique_ptr<reflect_field_container_helper> helper)
    : m_name(name)
    , m_offset(offset)
    , m_element_size(element_size)
    , m_type(type)
    , m_super_type(super_type)
    , m_display_name(display_name)
    , m_description(description)
    , m_container_type(container_type)
    , m_container_helper(std::move(helper))
{
}

const char* reflect_field::get_name()
{
    return m_name.c_str();
}

const char* reflect_field::get_display_name()
{
    return m_display_name.c_str();
}

const char* reflect_field::get_description()
{
    return m_description.c_str();
}

size_t reflect_field::get_offset()
{
    return m_offset;
}

size_t reflect_field::get_element_size()
{
    return m_element_size;
}

std::type_index reflect_field::get_type_index()
{
    return m_type;
}

std::type_index reflect_field::get_super_type_index()
{
    return m_super_type;
}

reflect_field_container_type reflect_field::get_container_type()
{
    return m_container_type;
}

reflect_field_container_helper* reflect_field::get_container_helper()
{
    return m_container_helper.get();
}

void reflect_field::add_constraint(std::unique_ptr<reflect_constraint> constraint)
{
    m_constraints.push_back(std::move(constraint));
}

}; // namespace workshop
