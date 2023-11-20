// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/containers/command_queue.h"
#include "workshop.core/memory/memory_tracker.h"

namespace ws {

command_queue::command_queue(size_t capacity)
{
    memory_scope scope(memory_type::engine__command_queue, memory_scope::k_ignore_asset);
    m_buffer.resize(capacity);
}

command_queue::~command_queue()
{
}

void command_queue::reset()
{
    m_write_offset = 0;
    m_command_head.store(nullptr);
    m_command_tail.store(nullptr);
}

bool command_queue::empty()
{
    return (m_command_head.load() == nullptr);
}

size_t command_queue::size_bytes()
{
    return m_write_offset.load();
}

std::span<uint8_t> command_queue::allocate_raw(size_t size)
{
    size_t offset = m_write_offset.fetch_add(size);
    if (offset + size > m_buffer.size())
    {
        db_fatal(core, "Ran out of space in command queue.");
    }
    return { m_buffer.data() + offset, size };
}

const char* command_queue::allocate_copy(const char* value)
{
    size_t required_space = strlen(value) + 1;
    std::span<uint8_t> buffer = allocate_raw(required_space);
    memcpy(buffer.data(), value, required_space);
    return reinterpret_cast<const char*>(buffer.data());
}

void command_queue::execute_next()
{
    command_header* header = m_command_head.load();
    db_assert_message(header, "Command queue is empty.");
    m_command_head.store(header->next);
    header->execute(header->lambda_pointer);
}

}; // namespace workshop
