// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "thirdparty/yamlcpp/include/yaml-cpp/yaml.h"

#include <array>
#include <vector>
#include <stdarg.h>
#include <string>

namespace ws {

// ================================================================================================
//  General purpose yaml serialization functions.
//  Add specializations for custom types.
// ================================================================================================

template<typename type>
inline void yaml_serialize(YAML::Node& out, bool is_loading, type& value)
{
    db_assert_message(false, "No serialize specialization for data type '%s'.", typeid(type).name());
}

// Handy functions for emitting yaml for various data types;
template<>
inline void yaml_serialize(YAML::Node& out, bool is_loading, int& value)
{
    if (is_loading)
    {
        if (out.IsDefined())
        {
            value = out.as<int>();
        }
    }
    else
    {
        out = value;
    }
}

template<>
inline void yaml_serialize(YAML::Node& out, bool is_loading, size_t& value)
{
    if (is_loading)
    {
        if (out.IsDefined())
        {
            value = out.as<size_t>();
        }
    }
    else
    {
        out = value;
    }
}

template<>
inline void yaml_serialize(YAML::Node& out, bool is_loading, float& value)
{
    if (is_loading)
    {
        if (out.IsDefined())
        {
            value = out.as<float>();
        }
    }
    else
    {
        out = value;
    }
}

template<>
inline void yaml_serialize(YAML::Node& out, bool is_loading, bool& value)
{
    if (is_loading)
    {
        if (out.IsDefined())
        {
            value = out.as<bool>();
        }
    }
    else
    {
        out = value;
    }
}

template<>
inline void yaml_serialize(YAML::Node& out, bool is_loading, std::string& value)
{
    if (is_loading)
    {
        if (out.IsDefined())
        {
            value = out.as<std::string>();
        }
    }
    else
    {
        out = value;
    }
}


/*
void yaml_write(YAML::Emitter& emitter, const vector3& value);
void yaml_write(YAML::Emitter& emitter, const quat& value);
void yaml_write(YAML::Emitter& emitter, const color& value);
*/

}; // namespace workshop
