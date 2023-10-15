// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/reflection/reflect.h"
#include "workshop.core/reflection/reflect_class.h"
#include "workshop.core/reflection/reflect_enum.h"

#include <algorithm>
#include <unordered_map>

namespace ws {
namespace {

std::unordered_map<std::type_index, reflect_class*>& get_class_reflect_map()
{
    static std::unordered_map<std::type_index, reflect_class*> g_reflect_classes;
    return g_reflect_classes;
}

std::unordered_map<std::type_index, reflect_enum*>& get_enum_reflect_map()
{
    static std::unordered_map<std::type_index, reflect_enum*> g_reflect_classes;
    return g_reflect_classes;
}

};

reflect_class* get_reflect_class(std::type_index index)
{
    std::unordered_map<std::type_index, reflect_class*>& map = get_class_reflect_map();
    if (auto iter = map.find(index); iter != map.end())
    {
        return iter->second;
    }
    return nullptr;
}

reflect_class* get_reflect_class(const char* name)
{
    std::unordered_map<std::type_index, reflect_class*>& map = get_class_reflect_map();
    for (auto& [index, cls] : map)
    {
        if (strcmp(cls->get_name(), name) == 0)
        {
            return cls;
        }
    }
    return nullptr;
}

std::vector<reflect_class*> get_reflect_derived_classes(std::type_index parent)
{
    std::vector<reflect_class*> ret;

    reflect_class* parent_class = get_reflect_class(parent);
    if (parent_class == nullptr)
    {
        return ret;
    }

    std::unordered_map<std::type_index, reflect_class*>& map = get_class_reflect_map();
    for (auto& [index, cls] : map)
    {
        if (cls->is_derived_from(parent_class))
        {
            ret.push_back(cls);
        }
    }

    return ret;
}

void register_reflect_class(reflect_class* object)
{
    std::unordered_map<std::type_index, reflect_class*>& map = get_class_reflect_map();
    map[object->get_type_index()] = object;
}

void unregister_reflect_class(reflect_class* object)
{
    std::unordered_map<std::type_index, reflect_class*>& map = get_class_reflect_map();
    map.erase(map.find(object->get_type_index()));
}

reflect_enum* get_reflect_enum(std::type_index index)
{
    std::unordered_map<std::type_index, reflect_enum*>& map = get_enum_reflect_map();
    if (auto iter = map.find(index); iter != map.end())
    {
        return iter->second;
    }
    return nullptr;
}

reflect_enum* get_reflect_enum(const char* name)
{
    std::unordered_map<std::type_index, reflect_enum*>& map = get_enum_reflect_map();
    for (auto& [index, cls] : map)
    {
        if (strcmp(cls->get_name(), name) == 0)
        {
            return cls;
        }
    }
    return nullptr;
}

void register_reflect_enum(reflect_enum* object)
{
    std::unordered_map<std::type_index, reflect_enum*>& map = get_enum_reflect_map();
    map[object->get_type_index()] = object;
}

void unregister_reflect_enum(reflect_enum* object)
{
    std::unordered_map<std::type_index, reflect_enum*>& map = get_enum_reflect_map();
    map.erase(map.find(object->get_type_index()));
}

}; // namespace workshop
