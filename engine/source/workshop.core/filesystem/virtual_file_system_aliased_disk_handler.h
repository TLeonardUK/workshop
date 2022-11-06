// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/filesystem/virtual_file_system_handler.h"

#include <string>
#include <filesystem>
#include <unordered_map>
#include <shared_mutex>

namespace ws {

// ================================================================================================
//  The aliased disk handler works similarly to the standard disk handler except that
//  the virtual paths are manually mapped to locations on disk via calls to alias() rather
//  than file system queries.
// 
//  This is useful for hiding complex paths behind user-friendly ones, such as when dealing
//  with file cache entries.
// 
//  This is always read-only.
// ================================================================================================
class virtual_file_system_aliased_disk_handler : public virtual_file_system_handler
{
public:

    virtual_file_system_aliased_disk_handler(bool read_only);

    virtual std::unique_ptr<stream> open(const char* path, bool for_writing) override;
    virtual virtual_file_system_path_type type(const char* path) override;
    virtual bool remove(const char* path) override;
    virtual bool modified_time(const char* path, virtual_file_system_time_point& timepoint) override;
    virtual std::vector<std::string> list(const char* path, virtual_file_system_path_type type) override;

    void alias(const char* virtual_path, const std::filesystem::path& disk_path);

protected:

    bool get_disk_path(const char* virtual_path, std::filesystem::path& disk_path);

private:
    std::shared_mutex m_alias_mutex;
    std::unordered_map<std::string, std::filesystem::path> m_aliases;

    std::string m_root;

    bool m_read_only;

};

}; // namespace workshop
