// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "thirdparty/nlohmann/json.hpp"

namespace ws {

// ================================================================================================
//  Template used for serializing a given value into a json file. A specialization
//  of this class should exist for each value type used.
// ================================================================================================
template <typename value_type>
struct json_value_serializer
{
    static void serialize(nlohmann::json& json, bool is_loading, const std::string& key, value_type& value)
    {
        if (is_loading)
        {
            if (json.contains(key))
            {
                value = (value_type)json[key];
            }
        }
        else
        {
            json[key] = value;
        }
    }
};

// Gets the parent node in a json object by path.
//
// The path: category1/category2/myvalue
// Will return the node: category1/category2
nlohmann::json* json_get_parent_by_name(nlohmann::json& input, const std::string& path, bool create_if_doesnt_exist = false);

// Gets the base node name from a json path.
//
// The path: category1/category2/myvalue
// Will return: myvalue
std::string json_get_node_name(const std::string& path);

}; // namespace workshop
