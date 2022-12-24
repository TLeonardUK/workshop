// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/renderer.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_param_block_archetype.h"
#include "workshop.render_interface/ri_param_block.h"

#include "workshop.core/hashing/hash.h"

namespace ws {

render_param_block_manager::render_param_block_manager(renderer& render)
    : m_renderer(render)
{
}

void render_param_block_manager::register_init(init_list& list)
{
}

render_param_block_manager::param_block_archetype_id render_param_block_manager::register_param_block_archetype(const char* name, ri_data_scope scope, const ri_data_layout& layout)
{
    std::scoped_lock lock(m_resource_mutex);

    // Generate a hash for the archetype so we can 
    // do a quick look up to determine if it already exists.
    size_t hash = 0;
    hash_combine(hash, scope);
    for (const ri_data_layout::field& field : layout.fields)
    {
        hash_combine(hash, field.data_type);
        hash_combine(hash, field.name);
    }

    for (auto& pair : m_param_block_archetypes)
    {
        if (pair.second.hash == hash)
        {
            pair.second.register_count++;
            return pair.first;
        }
    }

    param_block_state state;
    state.name = name;
    state.hash = hash;
    state.register_count = 1;

    ri_param_block_archetype::create_params params;
    params.layout = layout;
    params.scope = scope;

    state.instance = m_renderer.get_render_interface().create_param_block_archetype(params, name);
    if (state.instance == nullptr)
    {
        db_error(asset, "Failed to create param block archetype '%s'.", name);
        return invalid_id;
    }

    param_block_archetype_id id = m_id_counter++;
    m_param_block_archetypes[id] = std::move(state);
    m_param_block_archetype_by_name[name] = id;

    return id;
}

void render_param_block_manager::unregister_param_block_archetype(param_block_archetype_id id)
{
    std::scoped_lock lock(m_resource_mutex);

    if (auto iter = m_param_block_archetypes.find(id); iter != m_param_block_archetypes.end())
    {
        if (--iter->second.register_count == 0)
        {
            if (auto by_name_iter = m_param_block_archetype_by_name.find(iter->second.name); by_name_iter != m_param_block_archetype_by_name.end())
            {
                m_param_block_archetype_by_name.erase(by_name_iter);
            }

            m_param_block_archetypes.erase(iter);
        }
    }
}

ri_param_block_archetype* render_param_block_manager::get_param_block_archetype(param_block_archetype_id id)
{
    std::scoped_lock lock(m_resource_mutex);

    if (auto iter = m_param_block_archetypes.find(id); iter != m_param_block_archetypes.end())
    {
        return iter->second.instance.get();
    }
    return nullptr;    
}

std::unique_ptr<ri_param_block> render_param_block_manager::create_param_block(const char* name)
{
    std::scoped_lock lock(m_resource_mutex);

    if (auto iter = m_param_block_archetype_by_name.find(name); iter != m_param_block_archetype_by_name.end())
    {
        param_block_archetype_id id = iter->second;
        param_block_state& state = m_param_block_archetypes[id];
        return state.instance->create_param_block();
    }
    else
    {
        db_fatal(renderer, "Failed to create param block. Param block archetype '%s' isn't registered.");
        return nullptr;
    }
}

}; // namespace ws
