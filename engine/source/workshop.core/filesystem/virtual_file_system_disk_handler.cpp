// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/virtual_file_system_disk_handler.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/filesystem/disk_stream.h"
#include "workshop.core/filesystem/path_watcher.h"

#include <filesystem>

namespace ws {

virtual_file_system_disk_handler::virtual_file_system_disk_handler(const std::string& root, bool read_only)
    : m_root(root)
    , m_read_only(read_only)
{
    m_path_watcher = watch_path(root);
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

bool virtual_file_system_disk_handler::remove(const char* path)
{
    if (m_read_only)
    {
        return false;
    }

    std::filesystem::path fspath(m_root + "/" + path);
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

bool virtual_file_system_disk_handler::rename(const char* source, const char* destination)
{
    if (m_read_only)
    {
        return false;
    }

    std::filesystem::path source_fspath(m_root + "/" + source);
    std::filesystem::path dest_fspath(m_root + "/" + destination);

    if ( std::filesystem::exists(source_fspath) &&
        !std::filesystem::exists(dest_fspath))
    {
        try
        {
            std::filesystem::rename(source_fspath, dest_fspath);
            return true;
        }
        catch (std::filesystem::filesystem_error)
        {
            return false;
        }
    }

    return false;
}

bool virtual_file_system_disk_handler::create_directory(const char* path)
{
    if (m_read_only)
    {
        return false;
    }

    std::filesystem::path fspath(m_root + "/" + path);
    if (!std::filesystem::exists(fspath))
    {        
        try
        {
            return std::filesystem::create_directories(fspath);
        }
        catch (std::filesystem::filesystem_error)
        {
            return false;
        }
    }
    return true;
}

bool virtual_file_system_disk_handler::get_disk_location(const char* path, std::string& output_path)
{
    output_path = (m_root + "/" + path);
    return true;
}

bool virtual_file_system_disk_handler::modified_time(const char* path, virtual_file_system_time_point& timepoint)
{
    std::filesystem::path fspath(m_root + "/" + path);
    if (std::filesystem::exists(fspath))
    {
        std::filesystem::file_time_type time = std::filesystem::last_write_time(fspath);
        timepoint = std::chrono::file_clock::to_utc(time);

        return true;
    }

    return false;
}

std::vector<std::string> virtual_file_system_disk_handler::list(const char* path, virtual_file_system_path_type type, bool recursive)
{
    std::vector<std::string> result;

    std::filesystem::path fspath(m_root + "/" + path);

    if (std::filesystem::is_directory(fspath))
    {
        auto handle_path = [path, type, recursive, &result, &fspath](const std::filesystem::path& in_path, bool is_dir, bool is_file) {

            std::string abs_filename = std::string(path) + "/" + std::filesystem::relative(in_path, fspath).string();
            std::string normalized_filename = virtual_file_system::normalize(abs_filename.c_str());

            if (static_cast<int>(type) & static_cast<int>(virtual_file_system_path_type::directory))
            {
                if (is_dir)
                {
                    result.push_back(normalized_filename);
                }
            }
            else if (static_cast<int>(type) & static_cast<int>(virtual_file_system_path_type::file))
            {
                if (is_file)
                {
                    result.push_back(normalized_filename);
                }
            }
        };

        if (recursive)
        {
            for (auto iter = std::filesystem::recursive_directory_iterator(fspath); iter != std::filesystem::recursive_directory_iterator(); iter++)
            {
                handle_path(iter->path(), iter->is_directory(), iter->is_regular_file() || iter->is_symlink());
            }
        }
        else
        {
            for (auto iter = std::filesystem::directory_iterator(fspath); iter != std::filesystem::directory_iterator(); iter++)
            {
                handle_path(iter->path(), iter->is_directory(), iter->is_regular_file() || iter->is_symlink());
            }
        }
    }

    return result;

}

virtual_file_system_disk_watcher::~virtual_file_system_disk_watcher()
{
    std::scoped_lock lock(m_handler->m_registered_watchers_mutex);

    auto iter = std::find(m_handler->m_registered_watchers.begin(), m_handler->m_registered_watchers.end(), this);
    if (iter != m_handler->m_registered_watchers.end())
    {
        m_handler->m_registered_watchers.erase(iter);
    }
}

std::unique_ptr<virtual_file_system_watcher> virtual_file_system_disk_handler::watch(const char* path, virtual_file_system_watcher::callback_t callback)
{
    std::scoped_lock lock(m_registered_watchers_mutex);

    std::unique_ptr<virtual_file_system_disk_watcher> result = std::make_unique<virtual_file_system_disk_watcher>();
    result->m_callback = callback;
    result->m_path = virtual_file_system::normalize(path);
    result->m_handler = this;
    result->m_is_directory = false;

    try
    {
        result->m_is_directory = std::filesystem::is_directory(m_root + "/" + result->m_path);
    }
    catch (std::filesystem::filesystem_error)
    {
        // Doesn't exist yet perhaps?
    }

    m_registered_watchers.push_back(result.get());

    return result;
}

void virtual_file_system_disk_handler::raise_watch_events()
{
    std::scoped_lock lock(m_registered_watchers_mutex);

    if (m_path_watcher == nullptr)
    {
        return;
    }

    path_watcher::event evt;
    while (m_path_watcher->get_next_change(evt))
    {
        try
        {
            std::string relative_path = virtual_file_system::normalize(std::filesystem::relative(evt.path, m_root).string().c_str());

            for (virtual_file_system_disk_watcher* watcher : m_registered_watchers)
            {
                bool matches = false;

                // If directory match anything below it.
                if (watcher->m_is_directory && relative_path.starts_with(watcher->m_path))
                {
                    matches = true;
                }
                // Match path exactly.
                else if (watcher->m_path == relative_path)
                {
                    matches = true;
                }

                if (matches)
                {
                    watcher->m_callback(relative_path.c_str());
                }
            }
        }
        catch (std::filesystem::filesystem_error)
        {
            // File might have been removed causing the calls above to fail.
        }
    }
}

}; // namespace workshop
