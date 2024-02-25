// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/object.h"
#include "workshop.engine/ecs/object_manager.h"

namespace ws {

// Allows you to make an entry in the component_filter a negative - meaning an object will only
// be matched if it excludes the given component.
template <typename component_type>
struct excludes
{
    using is_exclude = void;
    using type = component_type;
};

// ================================================================================================ 
//  A component filter allows you to retrieve a list of all objects and their associated components
//  that match the filter parameters.
// 
//  The template argument allows some basic logic to select objects:
// 
//      component_filter<transform_component, camera_component> cameras_filter;
//      component_filter<transform_component, excludes<camera_component>> no_cameras_filter;
// 
// ================================================================================================
template <typename ...component_types>
class component_filter
{
private:
    template <typename a>
    void filter_type(const a& value, std::vector<std::type_index>& include_list, std::vector<std::type_index>& exclude_list)
    {
        include_list.push_back(typeid(a));
    }
    template <typename a>
    void filter_type(excludes<a> value, std::vector<std::type_index>& include_list, std::vector<std::type_index>& exclude_list)
    {
        exclude_list.push_back(typeid(a));
    }

    template <typename ...a>
    typename std::enable_if<sizeof...(a) == 0>::type unpack_types(std::vector<std::type_index>& include_list, std::vector<std::type_index>& exclude_list)
    {
    }

    template <typename a, typename ...b>
    void unpack_types(std::vector<std::type_index>& include_list, std::vector<std::type_index>& exclude_list)
    {
        a value = {};
        filter_type(value, include_list, exclude_list);
        unpack_types<b...>(include_list, exclude_list);
    }

public: 
    component_filter(object_manager& manager)
        : m_manager(manager)
    {
        std::vector<std::type_index> includes;
        std::vector<std::type_index> excludes;
        unpack_types<component_types...>(includes, excludes);

        m_archetype = m_manager.get_filter_archetype(includes, excludes);
    }

    // Gets number of elements.
    size_t size()
    {
        return m_archetype->size();
    }

    // Gets the object at the given index.
    object get_object(size_t index)
    {
        return m_archetype->get_object(index);
    }

    // Gets the component at the given index.
    template<typename component_type>
    component_type* get_component(size_t index)
    {
        return static_cast<component_type*>(m_archetype->get_component(index, typeid(component_type)));
    }

private:
    object_manager& m_manager;

    component_filter_archetype* m_archetype;

};

}; // namespace ws
