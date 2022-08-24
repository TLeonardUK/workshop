// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/containers/byte_queue.h"

namespace ws {

byte_queue::~byte_queue()
{
    empty();
}

size_t byte_queue::pending_bytes()
{
    return m_pending_bytes;
}

bool byte_queue::peek(uint8_t* buffer, size_t length)
{
    std::scoped_lock lock(m_mutex);

    if (m_pending_bytes < length)
    {
        return false;
    }

    block* read_block = m_queued_list_head;

    size_t bytes_remaining = length;
    while (bytes_remaining > 0)
    {
        size_t chunk_remaining = read_block->write_index - read_block->read_index;
        size_t copy_length = std::min(bytes_remaining, chunk_remaining);

        memcpy(buffer, read_block->buffer.data() + read_block->read_index, copy_length);

        buffer += copy_length;
        bytes_remaining -= copy_length;

        size_t new_read_index = read_block->read_index + copy_length;

        // Move to next block in the queue for reading.
        if (new_read_index == read_block->write_index)
        {
            read_block = read_block->next;
        }
    }

    return true;
}

bool byte_queue::dequeue(uint8_t* buffer, size_t length)
{
    std::scoped_lock lock(m_mutex);

    if (m_pending_bytes < length)
    {
        return false;
    }

    size_t bytes_remaining = length;
    while (bytes_remaining > 0)
    {
        block* read_block = m_queued_list_head;
        size_t chunk_remaining = read_block->write_index - read_block->read_index;
        size_t copy_length = std::min(bytes_remaining, chunk_remaining);

        memcpy(buffer, read_block->buffer.data() + read_block->read_index, copy_length);
        
        buffer += copy_length;
        bytes_remaining -= copy_length;
        read_block->read_index += copy_length;

        // Read all the data from the block, remove it from queue
        // and move it over to free list.
        if (read_block->read_index == read_block->write_index)
        {
            if (m_queued_list_head == m_queued_list_tail)
            {
                m_queued_list_tail = nullptr;
            }

            db_assert(m_queued_list_head->next != nullptr || bytes_remaining == 0);

            m_queued_list_head = m_queued_list_head->next;
            
            block* free_list_next = m_free_list;
            m_free_list = read_block;
            m_free_list->next = free_list_next;
        }
    }

    m_pending_bytes -= length;

    return true;
}

void byte_queue::enqueue(const uint8_t* buffer, size_t length)
{
    std::scoped_lock lock(m_mutex);

    size_t bytes_remaining = length;
    while (bytes_remaining > 0)
    {
        block* write_block = m_queued_list_tail;

        // If no block in queue, or at end of buffer for current write block, 
        // then allocate a new block for writing.
        if (write_block == nullptr || write_block->write_index == k_block_length)
        {
            if (m_free_list != nullptr)
            {
                write_block = m_free_list;
                m_free_list = m_free_list->next;
            }
            else
            {
                write_block = new block();
            }

            write_block->read_index = 0;
            write_block->write_index = 0;
            write_block->next = nullptr;

            if (m_queued_list_tail)
            {
                m_queued_list_tail->next = write_block;
            }
            m_queued_list_tail = write_block;

            if (m_queued_list_head == nullptr)
            {
                m_queued_list_head = write_block;
            }
        }

        size_t chunk_remaining = k_block_length - write_block->write_index;
        size_t copy_length = std::min(bytes_remaining, chunk_remaining);

        memcpy(write_block->buffer.data() + write_block->write_index, buffer, copy_length);

        buffer += copy_length;
        bytes_remaining -= copy_length;
        write_block->write_index += copy_length;
    }

    m_pending_bytes += length;
}

void byte_queue::empty()
{
    std::scoped_lock lock(m_mutex);

    while (m_queued_list_head)
    {
        block* tmp = m_queued_list_head;
        m_queued_list_head = m_queued_list_head->next;
        delete tmp;
    }

    while (m_free_list)
    {
        block* tmp = m_free_list;
        m_free_list = m_free_list->next;
        delete tmp;
    }

    m_queued_list_tail = nullptr;

    m_pending_bytes = 0;
}

void byte_queue::check_validity()
{
    block* block = m_queued_list_head;

    // Check for infinite loops and check data available in blocks taillies up to pending amount.
    size_t total_data = 0;
    for (int i = 0; block != nullptr; i++)
    {
        db_assert(i < 10000); 

        total_data += (block->write_index - block->read_index);

        block = block->next;
    }

    db_assert(total_data == m_pending_bytes);

    assert_in_head_check(m_queued_list_tail);
}

void byte_queue::assert_in_head_check(block* in)
{
    if (in == nullptr)
    {
        return;
    }

    block* block = m_queued_list_head;
    while (block)
    {
        if (block == in)
        {
            return;
        }
        block = block->next;
    }

    db_assert(false);
}

}; // namespace workshop
