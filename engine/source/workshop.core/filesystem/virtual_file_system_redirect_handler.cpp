// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/virtual_file_system_redirect_handler.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/filesystem/disk_stream.h"

#include <filesystem>

namespace ws {

virtual_file_system_redirect_handler::virtual_file_system_redirect_handler(bool read_only)
    : m_read_only(read_only)
{
}

std::unique_ptr<stream> virtual_file_system_redirect_handler::open(const char* path, bool for_writing)
{
    std::string target_path;
    if (!get_target_path(path, target_path))
    {
        return nullptr;
    }

    return virtual_file_system::get().open(target_path.c_str(), for_writing);
}

virtual_file_system_path_type virtual_file_system_redirect_handler::type(const char* path)
{
    std::string target_path;
    if (!get_target_path(path, target_path))
    {
        return virtual_file_system_path_type::non_existant;
    }

    return virtual_file_system::get().type(target_path.c_str());
}

bool virtual_file_system_redirect_handler::remove(const char* path)
{
    std::string target_path;
    if (!get_target_path(path, target_path))
    {
        return {};
    }

    return virtual_file_system::get().remove(target_path.c_str());
}

bool virtual_file_system_redirect_handler::rename(const char* source, const char* destination)
{
    std::string source_target_path;
    std::string dest_target_path;
    if (!get_target_path(source, source_target_path) ||
        !get_target_path(source, dest_target_path))
    {
        return {};
    }

    return virtual_file_system::get().rename(source_target_path.c_str(), dest_target_path.c_str());
}

bool virtual_file_system_redirect_handler::create_directory(const char* path)
{
    std::string target_path;
    if (!get_target_path(path, target_path))
    {
        return {};
    }

    return virtual_file_system::get().create_directory(target_path.c_str());
}

bool virtual_file_system_redirect_handler::modified_time(const char* path, virtual_file_system_time_point& timepoint)
{
    std::string target_path;
    if (!get_target_path(path, target_path))
    {
        return {};
    }

    return virtual_file_system::get().modified_time(target_path.c_str(), timepoint);
}

std::vector<std::string> virtual_file_system_redirect_handler::list(const char* path, virtual_file_system_path_type type)
{
    std::string target_path;
    if (!get_target_path(path, target_path))
    {
        return {};
    }

    return virtual_file_system::get().list(target_path.c_str(), type);
}


void virtual_file_system_redirect_handler::alias(const char* virtual_path, const char* target_path)
{
    std::unique_lock lock(m_alias_mutex);

    m_aliases[virtual_path] = target_path;
}

bool virtual_file_system_redirect_handler::get_target_path(const char* virtual_path, std::string& target_path)
{
    std::shared_lock lock(m_alias_mutex);

    if (auto iter = m_aliases.find(virtual_path); iter != m_aliases.end())
    {
        target_path = iter->second;
        return true; 
    }

    return false;
}

}; // namespace workshop
