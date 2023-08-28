// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/reflection/reflect_class.h"
#include "workshop.core/reflection/reflect_field.h"
#include "workshop.core/reflection/reflect.h"

#include <algorithm>

#pragma optimize("", off)

namespace ws {

reflect_class::reflect_class(const char* name, std::type_index index, std::type_index parent, reflect_class_flags flags, const char* display_name)
    : m_name(name)
    , m_type_index(index)
    , m_parent_type_index(parent)
    , m_display_name(display_name)
    , m_flags(flags)
{
    register_reflect_class(this);
}

reflect_class::~reflect_class()
{
    unregister_reflect_class(this);
}

const char* reflect_class::get_name()
{
    return m_name.c_str();
}

const char* reflect_class::get_display_name()
{
    return m_display_name.c_str();
}

bool reflect_class::has_flag(reflect_class_flags flag)
{
    return (m_flags & flag) != reflect_class_flags::none;
}

std::type_index reflect_class::get_type_index()
{
    return m_type_index;
}

reflect_field* reflect_class::find_field(const char* name)
{
    auto iter = std::find_if(m_fields.begin(), m_fields.end(), [name](auto& field) {
        return strcmp(field->get_name(), name) == 0;
    });
    
    if (iter != m_fields.end())
    {
        return iter->get();
    }

    return nullptr;
}

std::vector<reflect_field*> reflect_class::get_fields()
{
    std::vector<reflect_field*> ret;
    for (auto& ptr : m_fields)
    {
        ret.push_back(ptr.get());
    }
    return ret;
}

void reflect_class::add_field(const char* name, size_t offset, std::type_index type, const char* display_name, const char* description)
{
    std::unique_ptr<reflect_field> field = std::make_unique<reflect_field>(name, offset, type, display_name, description);
    m_fields.push_back(std::move(field));
}

bool reflect_class::is_derived_from(reflect_class* parent)
{
    reflect_class* cls = get_reflect_class(m_parent_type_index);
    while (cls)
    {
        if (cls == parent)
        {
            return true;
        }
        cls = get_reflect_class(cls->m_parent_type_index);
    }
    return false;
}

reflect_class* reflect_class::get_parent()
{
    return get_reflect_class(m_parent_type_index);;
}

}; // namespace workshop
