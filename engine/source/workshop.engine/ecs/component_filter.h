// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/object.h"
#include "workshop.engine/ecs/object_manager.h"

namespace ws {

// ================================================================================================ 
//  A component filter allows you to retrieve a list of all objects and their associated components
//  that match the filter parameters.
// ================================================================================================
template <typename ...component_types>
class component_filter
{
private:
    template <typename ...a>
    typename std::enable_if<sizeof...(a) == 0>::type unpack_types(std::vector<std::type_index>& output)
    {
    }

    template <typename a, typename ...b>
    void unpack_types(std::vector<std::type_index>& output)
    {
        output.push_back(typeid(a));
        unpack_types<b...>(output);
    }

public: 
    component_filter(object_manager& manager)
        : m_manager(manager)
    {
        std::vector<std::type_index> type_indices;
        unpack_types<component_types...>(type_indices);

        m_archetype = m_manager.get_filter_archetype(type_indices);
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
