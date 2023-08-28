// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/reflection/reflect.h"
#include "workshop.core/reflection/reflect_class.h"

#include <algorithm>
#include <unordered_map>

namespace ws {
namespace {

std::unordered_map<std::type_index, reflect_class*>& get_reflect_map()
{
    static std::unordered_map<std::type_index, reflect_class*> g_reflect_classes;
    return g_reflect_classes;
}

};

reflect_class* get_reflect_class(std::type_index index)
{
    std::unordered_map<std::type_index, reflect_class*>& map = get_reflect_map();
    if (auto iter = map.find(index); iter != map.end())
    {
        return iter->second;
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

    std::unordered_map<std::type_index, reflect_class*>& map = get_reflect_map();
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
    std::unordered_map<std::type_index, reflect_class*>& map = get_reflect_map();
    map[object->get_type_index()] = object;
}

void unregister_reflect_class(reflect_class* object)
{
    std::unordered_map<std::type_index, reflect_class*>& map = get_reflect_map();
    map.erase(map.find(object->get_type_index()));
}

}; // namespace workshop
