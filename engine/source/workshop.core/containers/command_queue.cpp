// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/containers/command_queue.h"

namespace ws {

command_queue::command_queue(size_t capacity)
{
    m_buffer.resize(capacity);
}

command_queue::~command_queue()
{
}

void command_queue::reset()
{
    m_write_offset = 0;
    m_read_offset = 0;
    m_non_data_command_read = 0;
    m_non_data_command_written = 0;
}

bool command_queue::empty()
{
    return (m_non_data_command_read >= m_non_data_command_written);
}

size_t command_queue::size_bytes()
{
    return m_write_offset.load();
}

std::span<uint8_t> command_queue::allocate(size_t size)
{
    std::span<uint8_t> span = allocate_raw(size + sizeof(command_header));
    
    command_header* header = reinterpret_cast<command_header*>(span.data());
    header->type_id = k_data_command_id;
    header->size = size;

    return { span.data() + sizeof(command_header), size };
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
    std::span<uint8_t> buffer = allocate(required_space);
    memcpy(buffer.data(), value, required_space);
    return reinterpret_cast<const char*>(buffer.data());
}

void command_queue::skip_command()
{
    command_header* header = reinterpret_cast<command_header*>(m_buffer.data() + m_read_offset.load());
    consume(sizeof(command_header) + header->size);
}

command_queue::command_header& command_queue::peek_header()
{
    command_header* header = reinterpret_cast<command_header*>(m_buffer.data() + m_read_offset.load());
    return *header;
}

void command_queue::consume(size_t bytes)
{
    m_read_offset.fetch_add(bytes);
    db_assert(m_read_offset.load() <= m_write_offset.load());
}

}; // namespace workshop
