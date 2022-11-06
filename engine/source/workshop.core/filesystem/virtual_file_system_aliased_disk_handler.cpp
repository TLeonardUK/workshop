// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/virtual_file_system_aliased_disk_handler.h"
#include "workshop.core/filesystem/disk_stream.h"

#include <filesystem>

namespace ws {

virtual_file_system_aliased_disk_handler::virtual_file_system_aliased_disk_handler(bool read_only)
    : m_read_only(read_only)
{
}

std::unique_ptr<stream> virtual_file_system_aliased_disk_handler::open(const char* path, bool for_writing)
{
    std::filesystem::path fspath;
    if (!get_disk_path(path, fspath))
    {
        return nullptr;
    }

    if (for_writing)
    {
        return nullptr;
    }

    if (std::filesystem::is_regular_file(fspath))
    {
        std::unique_ptr<disk_stream> stream = std::make_unique<disk_stream>();
        if (stream->open(fspath, for_writing))
        {
            return stream;
        }
    }
    
    return nullptr;
}

virtual_file_system_path_type virtual_file_system_aliased_disk_handler::type(const char* path)
{
    std::filesystem::path fspath;
    if (!get_disk_path(path, fspath))
    {
        return virtual_file_system_path_type::non_existant;
    }

    std::filesystem::file_status status = std::filesystem::status(fspath);
    if (status.type() == std::filesystem::file_type::directory ||
        status.type() == std::filesystem::file_type::junction)
    {
        return virtual_file_system_path_type::directory;
    }
    else if (status.type() == std::filesystem::file_type::regular ||
             status.type() == std::filesystem::file_type::symlink)
    {
        return virtual_file_system_path_type::file;
    }

    return virtual_file_system_path_type::non_existant;
}

bool virtual_file_system_aliased_disk_handler::remove(const char* path)
{
    if (m_read_only)
    {
        return false;
    }

    std::filesystem::path fspath;
    if (!get_disk_path(path, fspath))
    {
        return false;
    }

    try
    {
        std::filesystem::remove(fspath);
    }
    catch (std::filesystem::filesystem_error)
    {
        return false;
    }

    return true;
}

bool virtual_file_system_aliased_disk_handler::modified_time(const char* path, virtual_file_system_time_point& timepoint)
{
    std::filesystem::path fspath;
    if (!get_disk_path(path, fspath))
    {
        return false;
    }

    if (std::filesystem::exists(fspath))
    {
        std::filesystem::file_time_type time = std::filesystem::last_write_time(fspath);
        timepoint = std::chrono::file_clock::to_utc(time);

        return true;
    }

    return false;
}

std::vector<std::string> virtual_file_system_aliased_disk_handler::list(const char* path, virtual_file_system_path_type type)
{
    // Not supported for aliased handlers.

    return { };
}


void virtual_file_system_aliased_disk_handler::alias(const char* virtual_path, const std::filesystem::path& disk_path)
{
    std::unique_lock lock(m_alias_mutex);

    m_aliases[virtual_path] = disk_path;
}

bool virtual_file_system_aliased_disk_handler::get_disk_path(const char* virtual_path, std::filesystem::path& disk_path)
{
    std::shared_lock lock(m_alias_mutex);

    if (auto iter = m_aliases.find(virtual_path); iter != m_aliases.end())
    {
        disk_path = iter->second;
        return true; 
    }

    return false;
}

}; // namespace workshop
