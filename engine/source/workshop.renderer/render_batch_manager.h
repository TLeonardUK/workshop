// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"
#include "workshop.core/hashing/hash.h"
#include "workshop.core/hashing/string_hash.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.renderer/render_effect.h"
#include "workshop.renderer/render_resource_cache.h"
#include "workshop.renderer/render_visibility_manager.h"
#include "workshop.render_interface/ri_param_block.h"

#include <unordered_map>
#include <string>

namespace ws {

class renderer;
class asset_manager;
class shader;
class render_object;
class model;
class material;
class ri_buffer;
enum class material_domain;

// ================================================================================================
//  This enum dictates what kind of things this batch is going to be used for rendering.
// ================================================================================================
enum class render_batch_usage
{
    static_mesh
};

// ================================================================================================
//  Contains all the identifying information for an individual batch. All instances in a batch
//  will contain the same key
// ================================================================================================
struct render_batch_key
{
    asset_ptr<model> model;
    asset_ptr<material> material;
    size_t mesh_index;
    material_domain domain;
    render_batch_usage usage;

    bool operator==(const render_batch_key& other) const
    {
        return model == other.model && 
               mesh_index == other.mesh_index &&
               material == other.material && 
               domain == other.domain &&
               usage == other.usage;
    }

    bool operator!=(const render_batch_key& other) const
    {
        return !operator==(other);
    }
};

// ================================================================================================
//  Represents an individual instances in a render batch.
// ================================================================================================
struct render_batch_instance
{
    // Key for which batch this instance should be sorted into.
    render_batch_key key;

    // Object this instance refers to.
    render_object* object;

    // Param block containing instance specific fields.
    ri_param_block* param_block;

    // Key to use for checking instance visibility for a given view.
    render_visibility_manager::object_id visibility_id;
};

}; // namespace ws

// ================================================================================================
//  Specialization hash for batch types.
// ================================================================================================
template<>
struct std::hash<ws::render_batch_key>
{
    std::size_t operator()(const ws::render_batch_key& k) const
    {
        size_t hash = 0;
        ws::hash_combine(hash, k.material);
        ws::hash_combine(hash, k.mesh_index);
        ws::hash_combine(hash, k.model);
        ws::hash_combine(hash, k.domain);
        ws::hash_combine(hash, k.usage);
        return hash;
    }
};

template<>
struct std::hash<ws::asset_ptr<ws::model>>
{
    std::size_t operator()(const ws::asset_ptr<ws::model>& k) const
    {
        size_t hash = 0;
        ws::hash_combine(hash, k.get_hash());
        return hash;
    }
};

template<>
struct std::hash<ws::asset_ptr<ws::material>>
{
    std::size_t operator()(const ws::asset_ptr<ws::material>& k) const
    {
        size_t hash = 0;
        ws::hash_combine(hash, k.get_hash());
        return hash;
    }
};

namespace ws {

// ================================================================================================
//  A unique render batch that buckets a set of renderable instances
//  with similar properties.
// ================================================================================================
class render_batch
{
public:
    render_batch(render_batch_key key, renderer& render);

    const render_batch_key& get_key();
    const std::vector<render_batch_instance>& get_instances();

    void clear();

    render_resource_cache& get_resource_cache();

private:
    friend class render_batch_manager;

    void add_instance(const render_batch_instance& instance);
    void remove_instance(const render_batch_instance& instance);

private:
    render_batch_key m_key;
    renderer& m_renderer;
    std::vector<render_batch_instance> m_instances;

    std::unique_ptr<render_resource_cache> m_resource_cache;

};

// ================================================================================================
//  Responsible for calculating and storing batching information.
// ================================================================================================
class render_batch_manager
{
public:

    render_batch_manager(renderer& render);

    // Registers all the steps required to initialize the system.
    void register_init(init_list& list);

    // Called at the start of a frame.
    void begin_frame();

    // Registers a render instances and inserts it into an active batch.
    void register_instance(const render_batch_instance& instance);

    // Unregisters a render instance and removes it from all active batches.
    void unregister_instance(const render_batch_instance& instance);

    // Gets all the batches that have the given domain and usage.
    std::vector<render_batch*> get_batches(material_domain domain, render_batch_usage usage);

    // Invalidates any cached state that uses the given materail.
    void clear_cached_material_data(material* material);

protected:
    
    // Finds or creates a batch that uses the given key.
    render_batch* find_or_create_batch(render_batch_key key);

private:

    renderer& m_renderer;

    std::unordered_map<render_batch_key, std::unique_ptr<render_batch>> m_batches;

};

}; // namespace ws
