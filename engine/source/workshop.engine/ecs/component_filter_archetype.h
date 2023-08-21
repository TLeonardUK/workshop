// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/containers/sparse_vector.h"
#include "workshop.engine/ecs/object.h"
#include "workshop.engine/ecs/object_manager.h"

namespace ws {

// ================================================================================================ 
//  A component_filter_archetype stores all objects that match a specific filter, its used directly
//  by a component_filter to extract the needed information without recalculating what entities
//  pass the filter.
// ================================================================================================
class component_filter_archetype
{
public: 
    component_filter_archetype(object_manager& manager, const std::vector<std::type_index>& component_types);

    // Gets the number of objects mathcing the filter.
    size_t size();

    // Gets the object at the given index.
    object get_object(size_t index);

    // Gets the component at the given index.
    component* get_component(size_t index, std::type_index component_type);

    // Adds or updates the registration of a given object.
    void update_object(object handle);

    // Removes registration of a given object.
    void remove_object(object handle, bool ignore_match = false);

protected:

    // Checks if this object handle matches what this filter cares about.
    bool matches(object handle);

private:
    object_manager& m_manager;
    std::vector<std::type_index> m_component_types;

    struct object_info
    {
        object handle;
        std::vector<component*> components;
        size_t sort_key;
    };

    sparse_vector<object_info, memory_type::engine__ecs> m_object_info;

    // Determins an index into the m_object_info array based on the the object handle.
    std::unordered_map<object, size_t> m_object_info_lookup;

    // Indices into m_object_info sorted by spatial locality.
    std::vector<size_t> m_sorted_object_indices;

};

}; // namespace ws
