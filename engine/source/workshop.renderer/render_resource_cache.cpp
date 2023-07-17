// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_resource_cache.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.render_interface/ri_interface.h"

namespace ws {

render_resource_cache::render_resource_cache(renderer& renderer)
    : m_renderer(renderer)
{
}

render_batch_instance_buffer::render_batch_instance_buffer(renderer& render)
    : m_renderer(render)
{
    size_t pipeline_depth = m_renderer.get_render_interface().get_pipeline_depth();

    for (size_t i = 0; i < pipeline_depth; i++)
    {
        std::unique_ptr<buffer> buf = std::make_unique<buffer>();
        resize(*buf, 0);
        m_buffers.push_back(std::move(buf));
    }
}

void render_batch_instance_buffer::add(uint32_t table_index, uint32_t table_offset)
{
    buffer& buf = get_internal_buffer();
    
    resize(buf, buf.slots_in_use + 1);

    slot& slot_instance = buf.slots[buf.slots_in_use];

    if (slot_instance.table_index != table_index ||
        slot_instance.table_offset != table_offset)
    {
        slot_instance.table_index = table_index;
        slot_instance.table_offset = table_offset;
        slot_instance.dirty = true;
    }

    buf.slots_in_use++;
}

void render_batch_instance_buffer::commit()
{
    buffer& buf = get_internal_buffer();

    auto upload_range = [&buf](size_t start, size_t count) {

        size_t element_size = buf.buffer->get_element_size();

        void* ptr = buf.buffer->map(start * element_size, count * element_size);
        for (size_t j = start; j < start + count; j++)
        {
            slot& dirty_slot = buf.slots[j];

            instance_offset_info* offset_info = reinterpret_cast<instance_offset_info*>(ptr) + (j - start);
            offset_info->data_buffer_index = dirty_slot.table_index;
            offset_info->data_buffer_offset = dirty_slot.table_offset;

            dirty_slot.dirty = false;
        }
        buf.buffer->unmap(ptr);

    };

    // Find each block of dirty data and update it.
    size_t dirty_start = 0;
    bool in_dirty = false;

    for (size_t i = 0; i <= buf.slots.size(); i++)
    {
        if (i != buf.slots.size() && buf.slots[i].dirty)
        {
            if (!in_dirty)
            {
                dirty_start = i;
                in_dirty = true;
            }
        }
        else if (in_dirty)
        {
            upload_range(dirty_start, i - dirty_start);
            in_dirty = false;
        }
    }

    buf.slots_in_use = 0;
}

size_t render_batch_instance_buffer::size()
{
    return get_internal_buffer().slots.size();
}

size_t render_batch_instance_buffer::capacity()
{
    return get_internal_buffer().buffer->get_element_count();
}

ri_buffer& render_batch_instance_buffer::get_buffer()
{
    return *get_internal_buffer().buffer;
}

render_batch_instance_buffer::buffer& render_batch_instance_buffer::get_internal_buffer()
{
    size_t frame_index = m_renderer.get_frame_index();
    size_t pipeline_depth = m_renderer.get_render_interface().get_pipeline_depth();

    return *m_buffers[frame_index % pipeline_depth];
}

void render_batch_instance_buffer::resize(buffer& buf, size_t size)
{
    size_t old_size = buf.slots.size();

    if (buf.slots.size() < size)
    {
        size_t old_size = buf.slots.size();

        buf.slots.resize(size);

        for (size_t i = old_size; i < size; i++)
        {
            buf.slots[i].dirty = true;
        }
    }

    if (buf.buffer == nullptr || buf.buffer->get_element_count() < size)
    {
        size_t capacity = std::max(k_min_slot_count, size);
        if (buf.buffer != nullptr)
        {
            size_t growth = buf.buffer->get_element_count() * k_slot_growth_factor;
            capacity = std::max(growth, capacity);
        }

        ri_buffer::create_params create_params;
        create_params.element_count = capacity;
        create_params.element_size = sizeof(instance_offset_info);
        create_params.linear_data = {};
        create_params.usage = ri_buffer_usage::generic;

        buf.buffer = m_renderer.get_render_interface().create_buffer(create_params, "Instance Index Buffer");
    }
}

ri_param_block* render_resource_cache::find_param_block_by_name(const char* param_block_name)
{
    std::scoped_lock lock(m_mutex);

    auto iter = std::find_if(m_blocks.begin(), m_blocks.end(), [param_block_name](const param_block& block)
    {
        return param_block_name == block.name;
    });

    if (iter != m_blocks.end())
    {
        return iter->block.get();
    }

    return nullptr;
}

ri_param_block* render_resource_cache::find_or_create_param_block(
    void* key,
    const char* param_block_name,
    std::function<void(ri_param_block& block)> creation_callback)
{
    std::scoped_lock lock(m_mutex);

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

render_batch_instance_buffer* render_resource_cache::find_or_create_instance_buffer(void* key)
{
    std::scoped_lock lock(m_mutex);

    if (auto iter = m_instance_buffers.find(key); iter != m_instance_buffers.end())
    {
        return iter->second.buffer.get();
    }
    else
    {
        instance_buffer block;
        block.key = key;
        block.buffer = std::make_unique<render_batch_instance_buffer>(m_renderer);

        render_batch_instance_buffer* result = block.buffer.get();
        m_instance_buffers[key] = std::move(block);

        return result;
    }
}

void render_resource_cache::clear()
{
    std::scoped_lock lock(m_mutex);

    m_blocks.clear();
    m_instance_buffers.clear();
}

}; // namespace ws
