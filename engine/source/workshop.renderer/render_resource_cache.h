// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface/ri_param_block.h"

#include <typeindex>

namespace ws {

class renderer;
class asset_manager;
class shader;
class render_object;
class model;
class material;
class ri_buffer;
    
// ================================================================================================
//  Represents a buffer that holds offsets of instance param blocks associated with
//  a batch. 
// ================================================================================================
class render_batch_instance_buffer
{
public:
    render_batch_instance_buffer(renderer& render);

    void add(uint32_t table_index, uint32_t table_offset);
    void commit();
    size_t size();
    size_t capacity();

    ri_buffer& get_buffer();

private:
    struct slot
    {
        size_t table_index = 0;
        size_t table_offset = 0;
        bool dirty = false;
    };

    struct buffer
    {
        std::unique_ptr<ri_buffer> buffer;
        std::vector<slot> slots;
        size_t slots_in_use;
    };

    buffer& get_internal_buffer();
    void resize(buffer& buf, size_t size, bool create_backing_storage);

private:
    renderer& m_renderer;

    std::vector<std::unique_ptr<buffer>> m_buffers;

    // Minimum number of slots the buffer starts with.
    static inline constexpr size_t k_min_slot_count = 1;

    // Factor the buffer grows by.
    static inline constexpr size_t k_slot_growth_factor = 2;

};

// ================================================================================================
//  Simple cache that holds various types of rendering resources. Useful for caching view/batch/etc
//  specific resources in a somewhat elegant way.
// ================================================================================================
class render_resource_cache
{
public:
    render_resource_cache(renderer& renderer);

    // Finds a param block matching the given key, if one is not found, one is
    // created and the creation callback is called for it.
    ri_param_block* find_or_create_param_block(
        void* key,
        const char* param_block_name,
        std::function<void(ri_param_block& block)> creation_callback = {});

    // Finds the first block ith the given name.
    ri_param_block* find_param_block_by_name(const char* param_block_name);

    // Finds or creates an instance buffer with the matching key, if one is not
    // found a new one is created.
    render_batch_instance_buffer* find_or_create_instance_buffer(void* key);

    // Finds an arbitrary type of the given key, if one is not found, one is
    // created and the creation callback is called for it.
    template <typename data_type>
    data_type* find_or_create(
        void* key,
        std::function<auto() -> std::unique_ptr<data_type>> creation_callback)
    {
        std::scoped_lock lock(m_mutex);

        if (auto iter = m_untyped_values.find(key); iter != m_untyped_values.end())
        {
            db_assert(iter->second->type == typeid(data_type));
            return reinterpret_cast<data_type*>(iter->second->get_data());
        }
        else
        {
            std::unique_ptr<untyped_value<data_type>> block = std::make_unique<untyped_value<data_type>>();
            block->key = key;
            block->type = typeid(data_type);
            block->data = creation_callback();

            data_type* ret = block->data.get();

            m_untyped_values[key] = std::move(block);

            return ret;
        }
    }

    // Clears all data from the cache.
    void clear();

public:
    renderer& m_renderer;

    std::mutex m_mutex;

    struct param_block
    {
        void* key;
        std::string name;
        std::unique_ptr<ri_param_block> block;
    };
    std::vector<param_block> m_blocks;

    struct instance_buffer
    {
        void* key;
        std::unique_ptr<render_batch_instance_buffer> buffer;
    };
    std::unordered_map<void*, instance_buffer> m_instance_buffers;

    struct untyped_value_base
    {
    public:
        void* key;
        std::type_index type = typeid(void);

    public:
        untyped_value_base() = default;
        virtual ~untyped_value_base() = default;
        virtual void* get_data() = 0; 
    };

    template <typename data_type>
    struct untyped_value : public untyped_value_base
    {
    public:
        std::unique_ptr<data_type> data;

    public:
        virtual void* get_data() override
        {
            return data.get();
        }
    };

    std::unordered_map<void*, std::unique_ptr<untyped_value_base>> m_untyped_values;

};

}; // namespace ws
