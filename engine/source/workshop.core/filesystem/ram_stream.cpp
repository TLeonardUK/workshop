// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/ram_stream.h"

namespace ws {

ram_stream::ram_stream(std::vector<uint8_t>& buffer, bool can_write)
    : m_buffer(buffer)
    , m_can_write(can_write)
{
}

ram_stream::~ram_stream()
{
}

void ram_stream::close()
{
}

void ram_stream::flush()
{
}

bool ram_stream::can_write()
{
    return m_can_write;
}

size_t ram_stream::position()
{
    return m_position;
}

size_t ram_stream::length()
{
    return m_buffer.size();
}

void ram_stream::seek(size_t position)
{
    if (position > m_buffer.size())
    {
        m_buffer.resize(position);
    }
    m_position = position;
}

size_t ram_stream::write(const char* data, size_t size)
{
    if (!m_can_write)
    {
        db_assert_message(false, "Attempt to write to read only ram stream.");
        return 0;
    }

    size_t end_size = m_position + size;
    if (end_size > m_buffer.size())
    {
        m_buffer.resize(end_size);
    }

    memcpy(m_buffer.data() + m_position, data, size);
    m_position += size;

    return size;
}

size_t ram_stream::read(char* data, size_t size)
{
    if (m_position + size > m_buffer.size())
    {
        db_assert_message(false, "Attempt to read beyond bounds of ram stream.");
        return 0;
    }

    memcpy(data, m_buffer.data() + m_position, size);
    m_position += size;

    return size;
}

}; // namespace workshop
