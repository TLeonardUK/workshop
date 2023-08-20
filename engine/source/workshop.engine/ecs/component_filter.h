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
public: 
    component_filter(object_manager& manager)
        : m_manager(manager)
    {
        //m_archetype = m_manager.get_filter_archetype<component_types...>();
    }

    // Gets number of elements.
    size_t size()
    {
        return 0;
        //return m_archetype->size();
    }

    // Gets the object at the given index.
    object get_object(size_t index)
    {
        return null_object;
        //return m_archetype->get_object(index);
    }

    // Gets the component at the given index.
    template<typename component_type>
    component_type* get_component(size_t index)
    {
        return nullptr;
        //return m_archetype->get_component<component_type>(index);
    }

private:
    object_manager& m_manager;

};

}; // namespace ws
