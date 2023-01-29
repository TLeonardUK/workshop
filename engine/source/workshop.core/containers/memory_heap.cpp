// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/containers/memory_heap.h"

namespace ws {

memory_heap::memory_heap(size_t size)
{
    m_blocks.push_back({ 0, size, false });
}

memory_heap::~memory_heap()
{
}

bool memory_heap::alloc(size_t size, size_t alignment, size_t& offset)
{
    for (size_t i = 0; i < m_blocks.size(); i++)
    {
        block& curr_block = m_blocks[i];
        if (!curr_block.used && curr_block.size >= size)
        {
            size_t alignment_padding = math::round_up_multiple(curr_block.offset, alignment) - curr_block.offset;
            size_t size_required = size + alignment_padding;
            if (curr_block.size >= size_required)
            {
                offset = curr_block.offset + alignment_padding;
                curr_block.used = true;

                if (curr_block.size > size_required)
                {
                    block new_block;
                    new_block.offset = curr_block.offset + size_required;
                    new_block.size = curr_block.size - size_required;
                    new_block.used = false;

                    curr_block.size = size_required;

                    m_blocks.insert(m_blocks.begin() + i + 1, new_block);
                }

                return true;
            }
        }
    }

    return false;
}

bool memory_heap::get_block_index(size_t offset, size_t& index)
{
    for (size_t i = 0; i < m_blocks.size(); i++)
    {
        block& block = m_blocks[i];
        if (offset >= block.offset && offset < block.offset + block.size && block.used)
        {
            index = i;
            return true;
        }
    }
    return false;
}

bool memory_heap::empty()
{
    return (m_blocks.size() == 1 && !m_blocks[0].used);
}

void memory_heap::coalesce(size_t index)
{
    // Compact current block into previous block.
    while (index > 0 && index < m_blocks.size())
    {
        block& prev_block = m_blocks[index - 1];
        block& curr_block = m_blocks[index];

        if (!prev_block.used && !curr_block.used)
        {
            prev_block.size += curr_block.size;
            m_blocks.erase(m_blocks.begin() + index);
        }
        else
        {
            break;
        }
    }

    // Compact current block into next block.
    while (index >= 0 && index < m_blocks.size() - 1)
    {
        block& curr_block = m_blocks[index];
        block& next_block = m_blocks[index + 1];

        if (!curr_block.used && !next_block.used)
        {
            curr_block.size += next_block.size;
            m_blocks.erase(m_blocks.begin() + index + 1);
        }
        else
        {
            break;
        }
    }
}

void memory_heap::free(size_t offset)
{
    size_t index;
    if (get_block_index(offset, index))
    {
        m_blocks[index].used = false;
        coalesce(index);
    }
}

}; // namespace workshop
