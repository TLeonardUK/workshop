// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/assets/model/model.h"
#include "workshop.renderer/assets/material/material.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/common_types.h"

#include "workshop.render_interface/ri_interface.h"

#include "workshop.core/hashing/hash.h"

namespace ws {

render_batch_instance_buffer::render_batch_instance_buffer(renderer& render)
    : m_renderer(render)
{
}

void render_batch_instance_buffer::add(uint32_t table_index, uint32_t table_offset)
{
    size_t frame_index = m_renderer.get_frame_index();

    // If a slot already has this value from a previous frame, just mark it as active.
    // TODO: This is slow as fuck, please fix.
    size_t free_slot_index = 0;
    bool free_slot_found = false;

    for (size_t i = 0; i < m_slots.size(); i++)
    {
        slot& slot_instance = m_slots[i];
        if (slot_instance.last_frame_used < frame_index)
        {
            if (slot_instance.table_index == table_index &&
                slot_instance.table_offset == table_offset)
            {
                slot_instance.last_frame_used = frame_index;
                return;
            }
            else
            {
                free_slot_index = i;
                free_slot_found = true;
            }
        }
    }

    if (!free_slot_found)
    {
        free_slot_index = m_slots.size();
        resize(m_slots.size() + 1);
    }

    slot& slot_instance = m_slots[free_slot_index];
    slot_instance.dirty = true;
    slot_instance.table_index = table_index;
    slot_instance.table_offset = table_offset;
    slot_instance.last_frame_used = frame_index;
}

void render_batch_instance_buffer::commit()
{
    if (m_buffer == nullptr)
    {
        return;
    }

    size_t frame_index = m_renderer.get_frame_index();
    size_t element_size = m_buffer->get_element_size();

    // Defragment instance buffer by filling gaps.
    for (size_t i = 0; i < m_slots.size(); i++)
    {
        slot& slot_instance = m_slots[i];

        // Empty if not used this frame.
        if (slot_instance.last_frame_used < frame_index)
        {
            // Find next used slot and move it down.
            bool filled_slot = false;

            for (size_t j = i + 1; j < m_slots.size(); j++)
            {
                slot& other_slot = m_slots[j];

                if (other_slot.last_frame_used >= frame_index)
                {
                    slot_instance = other_slot;
                    slot_instance.dirty = true;

                    other_slot.last_frame_used = 0;
                    other_slot.dirty = false;

                    filled_slot = true;

                    break;
                }
            }

            // If nothing was found to fill this spot, just make it 
            // a free index again.
            if (!filled_slot)
            {
                slot_instance.dirty = false;
                slot_instance.last_frame_used = 0;
            }
        }
    }

    // Find out which regions of the buffer have changed and upload them.
    for (size_t i = 0; i < m_slots.size(); /* empty */)
    {
        slot& slot_instance = m_slots[i];

        // Start of a dirty region.
        if (slot_instance.dirty)
        {
            size_t dirty_end_index = m_slots.size();
            slot_instance.dirty = false;

            // Find end of region.
            for (size_t j = i + 1; j < m_slots.size(); j++)
            {
                slot& other_slot = m_slots[j];
                if (!other_slot.dirty)
                {
                    dirty_end_index = j;
                    break;
                }
                else
                {
                    other_slot.dirty = false;
                }
            }

            // Map and write all the data to the buffer.
            size_t dirty_element_count = dirty_end_index - i;
            void* ptr = m_buffer->map(i * element_size, dirty_element_count * element_size);
            for (size_t j = 0; j < dirty_element_count; j++)
            {
                slot& dirty_slot = m_slots[i + j];

                instance_offset_info* offset_info = reinterpret_cast<instance_offset_info*>(ptr) + j;
                offset_info->data_buffer_index = dirty_slot.table_index;
                offset_info->data_buffer_offset = dirty_slot.table_offset;
            }
            m_buffer->unmap(ptr);

            i = dirty_end_index;
        }
        else
        {
            i++;
        }
    }
}

size_t render_batch_instance_buffer::size()
{
    return m_slots.size();
}

size_t render_batch_instance_buffer::capacity()
{
    return (m_buffer == nullptr ? 0 : m_buffer->get_element_count());
}

ri_buffer& render_batch_instance_buffer::get_buffer()
{
    return *m_buffer.get();
}

void render_batch_instance_buffer::resize(size_t size)
{
    size_t old_size = m_slots.size();

    m_slots.resize(size);

    if (m_buffer == nullptr || m_buffer->get_element_count() < size)
    {
        size_t capacity = std::max(k_min_slot_count, size);
        if (m_buffer != nullptr)
        {
            size_t growth = m_buffer->get_element_count() * k_slot_growth_factor;
            capacity = std::max(growth, capacity);
        }

        ri_buffer::create_params create_params;
        create_params.element_count = capacity;
        create_params.element_size = sizeof(instance_offset_info);
        create_params.linear_data = {};
        create_params.usage = ri_buffer_usage::generic;

        m_buffer = m_renderer.get_render_interface().create_buffer(create_params, "Instance Index Buffer");
    }
}

render_batch::render_batch(render_batch_key key, renderer& render)
    : m_key(key)
    , m_renderer(render)
{
}

render_batch_key render_batch::get_key()
{
    return m_key;
}

const std::vector<render_batch_instance>& render_batch::get_instances()
{
    return m_instances;
}

void render_batch::clear()
{
    m_instances.clear();
}

ri_param_block* render_batch::find_or_create_param_block(
    void* key,
    const char* param_block_name,
    std::function<void(ri_param_block& block)> creation_callback)
{
    auto iter = std::find_if(m_blocks.begin(), m_blocks.end(), [key, param_block_name](const param_block& block)
    {
        return key == block.key && param_block_name == block.name;
    });

    if (iter != m_blocks.end())
    {
        return iter->block.get();
    }
    else
    {
        std::unique_ptr<ri_param_block> new_block = m_renderer.get_param_block_manager().create_param_block(param_block_name);

        db_log(renderer, "Creating new param block: %s", param_block_name);

        if (creation_callback)
        {
            creation_callback(*new_block.get());
        }

        ri_param_block* result = new_block.get();

        m_blocks.emplace_back(key, param_block_name, std::move(new_block));

        return result;
    }

/*
    if (auto iter = m_blocks.find(key); iter == m_blocks.end())
    {
        param_blocks block;
        block.key = key;
         
        m_blocks[key] = block;
    }

    param_blocks& blocks = m_blocks[key];
    if (auto iter = blocks.blocks.find(param_block_name); iter == blocks.blocks.end())
    {
        return iter->second.get();
    }
    else
    {
    }*/

    return nullptr;
}

render_batch_instance_buffer* render_batch::find_or_create_instance_buffer(void* key)
{
    if (auto iter = m_instance_buffers.find(key); iter != m_instance_buffers.end())
    {
        return iter->second.buffer.get();
    }
    else
    {
        db_log(renderer, "Creating new instance buffer: %p", key);

        instance_buffer block;
        block.key = key;
        block.buffer = std::make_unique<render_batch_instance_buffer>(m_renderer);

        render_batch_instance_buffer* result = block.buffer.get();
        m_instance_buffers[key] = std::move(block);

        return result;
    }
}

void render_batch::add_instance(const render_batch_instance& instance)
{
    m_instances.push_back(instance);
}

void render_batch::remove_instance(const render_batch_instance& instance)
{
    auto iter = std::find_if(m_instances.begin(), m_instances.end(), [&instance](const render_batch_instance& other) {
        return other.object == instance.object;
    });

    if (iter != m_instances.end())
    {
        m_instances.erase(iter);
    }
}

void render_batch::clear_cached_data()
{
    m_blocks.clear();
    m_instance_buffers.clear();
}

render_batch_manager::render_batch_manager(renderer& render)
    : m_renderer(render)
{
}

void render_batch_manager::register_init(init_list& list)
{
}

render_batch* render_batch_manager::find_or_create_batch(render_batch_key key)
{
    if (auto iter = m_batches.find(key); iter != m_batches.end())
    {
        return iter->second.get();
    }
    else
    {
        std::unique_ptr<render_batch> batch = std::make_unique<render_batch>(key, m_renderer);

        render_batch* batch_ptr = batch.get();
        m_batches[key] = std::move(batch);

        return batch_ptr;
    }
}

void render_batch_manager::begin_frame()
{
}

void render_batch_manager::register_instance(const render_batch_instance& instance)
{
    render_batch* batch = find_or_create_batch(instance.key);
    batch->add_instance(instance);
}

void render_batch_manager::unregister_instance(const render_batch_instance& instance)
{
    render_batch* batch = find_or_create_batch(instance.key);
    batch->remove_instance(instance);
}

std::vector<render_batch*> render_batch_manager::get_batches(material_domain domain, render_batch_usage usage)
{
    std::vector<render_batch*> result;

    for (auto& [key, batch] : m_batches)
    {
        render_batch_key key = batch->get_key();
        if (key.domain == domain && key.usage == usage)
        {
            result.push_back(batch.get());
        }
    }

    return result;
}

void render_batch_manager::clear_cached_material_data(material* material)
{
    for (auto& [key, batch] : m_batches)
    {
        render_batch_key key = batch->get_key();
        if (key.domain != material->domain)
        {
            continue;
        }

        if (!key.model.is_loaded())
        {
            continue;
        }

        model::material_info& material_info = key.model->materials[key.material_index];
        if (!material_info.material.is_loaded())
        {
            continue;
        }

        if (material_info.material.get() == material)
        {
            batch->clear_cached_data();
        }
    }
}

}; // namespace ws
