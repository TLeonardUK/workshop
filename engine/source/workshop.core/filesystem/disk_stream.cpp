// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/disk_stream.h"

namespace ws {

disk_stream::~disk_stream()
{
    if (m_file != nullptr)
    {
        close();
    }
}

result<void> disk_stream::open(const std::filesystem::path& path, bool for_writing)
{
    db_assert(m_file == nullptr);

    m_can_write = for_writing;

    m_path = path;

    m_file = fopen(path.string().c_str(), for_writing ? "wb" : "rb");
    if (m_file == nullptr)
    {
        return false;
    }

    // Cache the length of the file up front.
    _fseeki64(m_file, 0, SEEK_END);
    m_length = _ftelli64(m_file);
    _fseeki64(m_file, 0, SEEK_SET);

    return true;
}

void disk_stream::close()
{
    db_assert(m_file != nullptr);

    fclose(m_file);
    m_file = nullptr;
}

void disk_stream::flush()
{
    db_assert(m_file != nullptr);

    fflush(m_file);
}

bool disk_stream::can_write()
{
    return m_can_write;
}

size_t disk_stream::position()
{
    db_assert(m_file != nullptr);

    return _ftelli64(m_file);
}

size_t disk_stream::length()
{
    db_assert(m_file != nullptr);

    return m_length;
}

void disk_stream::seek(size_t position)
{
    db_assert(m_file != nullptr);

    _fseeki64(m_file, position, SEEK_SET);
}

size_t disk_stream::write(const char* data, size_t size)
{
    db_assert(m_file != nullptr);

    return fwrite(data, size, 1, m_file) * size;
}

size_t disk_stream::read(char* data, size_t size)
{
    db_assert(m_file != nullptr);

    return fread(data, size, 1, m_file) * size;
}

std::string disk_stream::get_async_path()
{
    return m_path.string().c_str();
}

size_t disk_stream::get_async_offset()
{
    return position();
}


}; // namespace workshop
