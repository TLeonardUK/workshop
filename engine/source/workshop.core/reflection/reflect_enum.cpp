// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/reflection/reflect_enum.h"
#include "workshop.core/reflection/reflect.h"

#include <algorithm>

namespace ws {

reflect_enum::reflect_enum(const char* name, std::type_index index, reflect_enum_flags flags, const char* display_name)
    : m_name(name)
    , m_type_index(index)
    , m_display_name(display_name)
    , m_flags(flags)
{
    register_reflect_enum(this);
}

reflect_enum::~reflect_enum()
{
    unregister_reflect_enum(this);
}

const char* reflect_enum::get_name()
{
    return m_name.c_str();
}

const char* reflect_enum::get_display_name()
{
    return m_display_name.c_str();
}

bool reflect_enum::has_flag(reflect_enum_flags flag)
{
    return (m_flags & flag) != reflect_enum_flags::none;
}

std::type_index reflect_enum::get_type_index()
{
    return m_type_index;
}

const reflect_enum::value* reflect_enum::find_value(const char* name)
{
    for (auto& val : m_values)
    {
        if (val->name == name)
        {
            return val.get();
        }
    }
    return nullptr;
}

const reflect_enum::value* reflect_enum::find_value(int64_t value)
{
    for (auto& val : m_values)
    {
        if (val->value == value)
        {
            return val.get();
        }
    }
    return nullptr;
}

std::vector<const reflect_enum::value*> reflect_enum::get_values()
{
    std::vector<const reflect_enum::value*> result;
    result.reserve(m_values.size());
    for (auto& val : m_values)
    {
        result.push_back(val.get());
    }
    return result;
}

void reflect_enum::add_value(
    const char* name, 
    int64_t numeric_value,
    const char* display_name, 
    const char* description)
{
    std::unique_ptr<value> new_value = std::make_unique<value>();
    new_value->description = description;
    new_value->display_name = display_name;
    new_value->name = name;
    new_value->value = numeric_value;

    m_values.push_back(std::move(new_value));
}

}; // namespace workshop
