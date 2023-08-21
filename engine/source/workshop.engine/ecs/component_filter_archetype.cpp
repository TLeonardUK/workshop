// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/ecs/component_filter_archetype.h"

namespace ws {

component_filter_archetype::component_filter_archetype(object_manager& manager, const std::vector<std::type_index>& component_types)
    : m_manager(manager)
    , m_component_types(component_types)
    , m_object_info(object_manager::k_max_objects)
{
}

size_t component_filter_archetype::size()
{
    return m_sorted_object_indices.size();
}

object component_filter_archetype::get_object(size_t index)
{
    size_t info_index = m_sorted_object_indices[index];
    object_info& info = m_object_info[info_index];
    return info.handle;
}

component* component_filter_archetype::get_component(size_t index, std::type_index component_type)
{
    size_t info_index = m_sorted_object_indices[index];
    object_info& info = m_object_info[info_index];

    for (size_t i = 0; i < m_component_types.size(); i++)
    {
        if (m_component_types[i] == component_type)
        {
            return info.components[i];
        }
    }

    return nullptr;
}

bool component_filter_archetype::matches(object handle)
{
    std::vector<component*> comps = m_manager.get_components(handle);
    for (std::type_index& required_type : m_component_types)
    {
        auto iter = std::find_if(comps.begin(), comps.end(), [required_type](component* val) {
            return typeid(*val) == required_type;
        });
        if (iter == comps.end())
        {
            return false;
        }
    }
    return true;
}

void component_filter_archetype::update_object(object handle)
{
    auto create_object_info = [this, handle]()
    {
        std::vector<component*> components = m_manager.get_components(handle);

        object_info info;
        info.components.resize(m_component_types.size());
        for (size_t i = 0; i < m_component_types.size(); i++)
        {
            std::type_index expected_type = m_component_types[i];

            auto iter = std::find_if(components.begin(), components.end(), [expected_type](const component* comp) {
                return expected_type == typeid(*comp);
            });
            if (iter == components.end())
            {
                db_fatal(engine, "Failed to find component of expected type.");
            }

            info.components[i] = *iter;
        }
        info.handle = handle;
        info.sort_key = reinterpret_cast<size_t>(info.components[0]);

        return info;
    };

    // See if entry already exists for object.
    auto iter = m_object_info_lookup.find(handle);
    if (iter != m_object_info_lookup.end())
    {
        // If it no longer matches out requirements remove it.
        if (!matches(handle))
        {
            remove_object(handle, true);
        }
        // Otherwise update existing entry.
        else
        {
            size_t info_index = iter->second;

            object_info& info = m_object_info[info_index];
            object_info new_info = create_object_info();

            size_t needs_resorting = (info.sort_key != new_info.sort_key);

            if (info.sort_key != new_info.sort_key)
            {
                auto iter = std::find(m_sorted_object_indices.begin(), m_sorted_object_indices.end(), info_index);
                m_sorted_object_indices.erase(iter);
                
                auto insert_iter = std::lower_bound(m_sorted_object_indices.begin(), m_sorted_object_indices.end(), info_index, [this](const size_t& k1, const size_t& k2) {
                    return m_object_info[k1].sort_key < m_object_info[k2].sort_key;
                });
                m_sorted_object_indices.insert(insert_iter, info_index);
            }
        }
    }

    // If new entry that matches our requirements, add a new entry.
    else if (matches(handle))
    {
        // Create new info entry and insert it into the lookup table.
        object_info info = create_object_info();

        size_t index = m_object_info.insert(info);
        m_object_info_lookup.emplace(handle, index);

        // Insert into sorted list based on sort key.
        auto insert_iter = std::lower_bound(m_sorted_object_indices.begin(), m_sorted_object_indices.end(), index, [this](const size_t& k1, const size_t& k2) {
            return m_object_info[k1].sort_key < m_object_info[k2].sort_key;
        });
        m_sorted_object_indices.insert(insert_iter, index);
    }
}

void component_filter_archetype::remove_object(object handle, bool ignore_match)
{
    if (!ignore_match && !matches(handle))
    {
        return;
    }

    auto iter = m_object_info_lookup.find(handle);
    if (iter != m_object_info_lookup.end())
    {
        size_t index = iter->second;

        m_object_info_lookup.erase(iter);
        m_object_info.remove(index);

        auto sorted_iter = std::find(m_sorted_object_indices.begin(), m_sorted_object_indices.end(), index);
        if (sorted_iter != m_sorted_object_indices.end())
        {
            m_sorted_object_indices.erase(sorted_iter);
        }
    }
}

}; // namespace ws
