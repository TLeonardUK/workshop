// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/virtual_file_system_disk_handler.h"
#include "workshop.core/filesystem/disk_stream.h"

#include <filesystem>

namespace ws {

virtual_file_system_disk_handler::virtual_file_system_disk_handler(const std::string& root, bool read_only)
    : m_root(root)
    , m_read_only(read_only)
{
}

std::unique_ptr<stream> virtual_file_system_disk_handler::open(const char* path, bool for_writing)
{
    std::filesystem::path fspath(m_root + "/" + path);

    if (for_writing)
    {
        if (m_read_only)
        {
            return nullptr;
        }

        std::filesystem::path dir = fspath.parent_path();
        if (!std::filesystem::exists(dir))
        {
            std::filesystem::create_directories(dir);
        }

        std::unique_ptr<disk_stream> stream = std::make_unique<disk_stream>();
        if (stream->open(fspath, for_writing))
        {
            return stream;
        }
    }
    else
    {
        if (std::filesystem::is_regular_file(fspath))
        {
            std::unique_ptr<disk_stream> stream = std::make_unique<disk_stream>();
            if (stream->open(fspath, for_writing))
            {
                return stream;
            }
        }
    }

    return nullptr;
}

virtual_file_system_path_type virtual_file_system_disk_handler::type(const char* path)
{
    std::filesystem::path fspath(m_root + "/" + path);

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

std::vector<std::string> virtual_file_system_disk_handler::list(const char* path, virtual_file_system_path_type type)
{
    std::vector<std::string> result;

    std::filesystem::path fspath(m_root + "/" + path);

    if (std::filesystem::is_directory(fspath))
    {
        for (auto iter = std::filesystem::directory_iterator(fspath); iter != std::filesystem::directory_iterator(); iter++)
        {
            if (static_cast<int>(type) & static_cast<int>(virtual_file_system_path_type::directory))
            {
                if (iter->is_directory())
                {
                    result.push_back(iter->path().filename().string().c_str());
                }
            }
            else if (static_cast<int>(type) & static_cast<int>(virtual_file_system_path_type::file))
            {
                if (iter->is_regular_file() ||
                    iter->is_symlink())
                {
                    result.push_back(iter->path().filename().string().c_str());
                }
            }
        }
    }

    return result;
}

}; // namespace workshop
