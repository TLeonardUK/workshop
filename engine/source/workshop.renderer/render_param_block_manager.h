// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"

#include "workshop.render_interface/ri_types.h"

namespace ws {

class renderer;
class ri_param_block_archetype;
class ri_param_block;

// ================================================================================================
//  Acts as a central repository for all loaded param block archtypes.
// ================================================================================================
class render_param_block_manager
{
public:

    // Identifies a parameter block archetype that has been registered to the renderer.
    using param_block_archetype_id = size_t;

    // Invalid id value for any of the above.
    constexpr static inline size_t invalid_id = 0;

public:

    render_param_block_manager(renderer& render);

    // Registers all the steps required to initialize the system.
    void register_init(init_list& list);

public:

    // Registers a param block archetype that may be used for renderering. If
    // a param block with the same layout/scope is available it will be returned.
    param_block_archetype_id register_param_block_archetype(const char* name, ri_data_scope scope, const ri_data_layout& layout);

    // Unregisters a previous param block archetype. This may not take immediately effect
    // if parts of the renderer are still using the param block archetype.
    void unregister_param_block_archetype(param_block_archetype_id id);

    // Swaps the internal data stored in each param block archetype id. Be careful using this
    // its meant primarily for supporting hot-reloading.
    void swap_param_block_archetype(param_block_archetype_id id, param_block_archetype_id other_id);

    // Gets a param block archetype from its id.
    ri_param_block_archetype* get_param_block_archetype(param_block_archetype_id id);

    // Shortcut for creating a param block from a registered archetype.
    std::unique_ptr<ri_param_block> create_param_block(const char* name);

private:

    struct param_block_state
    {
        std::string name;
        size_t register_count = 0;
        size_t hash = 0;
        std::unique_ptr<ri_param_block_archetype> instance;
    };

    std::recursive_mutex m_resource_mutex;

    size_t m_id_counter = 1;
    std::unordered_map<param_block_archetype_id, param_block_state> m_param_block_archetypes;
    std::unordered_map<std::string, param_block_archetype_id> m_param_block_archetype_by_name;

    renderer& m_renderer;

};

}; // namespace ws
