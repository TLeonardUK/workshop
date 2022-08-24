// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/debug/debug.h"
#include "workshop.core/debug/log_handler_file.h"

namespace ws {

log_handler_file::log_handler_file(const std::filesystem::path& root_directory, size_t file_count, size_t max_file_size)
    : m_file_directory(root_directory)
    , m_file_count(file_count)
    , m_max_file_size(max_file_size)
{
    // If directory doesn't exist, create it.
    if (!std::filesystem::is_directory(root_directory))
    {
        if (!std::filesystem::create_directories(root_directory))
        {
            // Not much we can do. Abort.
            return;
        }
    }

    // Grab a list of all the current logs files in the directory
    // we will use this for deciding what to write to.
    for (auto& entry : std::filesystem::directory_iterator(root_directory))
    {
        std::filesystem::path extension = entry.path().extension();
        if (extension == k_log_extension)
        {
            m_existing_files.push_back(entry.path());
        }
    }

    // Open newest file.
    open_target_file(false);

    m_active = true;
}

std::filesystem::path log_handler_file::get_target_file(bool no_append)
{
    // Check newest file, if it still has space, write to it.
    if (!no_append)
    {
        int file_index = get_newest_file_index();
        if (file_index >= 0)
        {
            std::filesystem::path path = m_existing_files[file_index];
            if (std::filesystem::file_size(path) < m_max_file_size)
            {
                return path;
            }
        }
    }

    // Otherwise make a new file.
    time_t current_time = time(0);
    struct tm current_time_tstruct;
    localtime_s(&current_time_tstruct, &current_time);

    char time_buffer[128];
    strftime(time_buffer, sizeof(time_buffer), "%Y%m%d_%H%M%S", &current_time_tstruct);

    char filename_buffer[128];
    snprintf(filename_buffer, 128, "%s%s", time_buffer, k_log_extension.string().c_str());

    return m_file_directory / filename_buffer;
}

void log_handler_file::open_target_file(bool no_append)
{
    std::filesystem::path new_path = get_target_file(no_append);
    
    if (m_current_file != nullptr)
    {
        fclose(m_current_file);
        m_current_file = nullptr;
    }

    bool is_new_file = !std::filesystem::exists(new_path);

    fopen_s(&m_current_file, new_path.string().c_str(), "a+c"); // c causes windows to commit when flushed.
    m_current_file_path = new_path;
    if (m_current_file != nullptr)
    {
        m_current_file_remaining_space = m_max_file_size - ftell(m_current_file);
    }
    if (is_new_file)
    {
        m_existing_files.push_back(new_path);
    }

    cull_old_files();

    // Be careful with this, if m_max_file_size is tiny this could potentially cause an
    // infinite loop.
    if (m_current_file != nullptr)
    {
        db_log(core, "Opened log file: %s", new_path.string().c_str());
    }
    // If we failed to open the file, deactivate this logger.
    else
    {
        m_active = false;
        db_error(core, "Failed to open log file: %s", new_path.string().c_str());
    }
}

void log_handler_file::cull_old_files()
{
    while (m_existing_files.size() > m_file_count)
    {
        int file_index = get_oldest_file_index();

        std::filesystem::path path = m_existing_files[file_index];
        std::filesystem::remove(path);

        db_log(core, "Removed old log file: %s", path.string().c_str());

        m_existing_files.erase(m_existing_files.begin() + file_index);
    }
}

int log_handler_file::get_newest_file_index()
{
    int result = -1; 
    std::filesystem::file_time_type result_timestamp;

    for (int i = 0; i < m_existing_files.size(); i++)
    {
        std::filesystem::file_time_type timestamp = std::filesystem::last_write_time(m_existing_files[i]);
        if (i == 0 || timestamp > result_timestamp)
        {
            result = i;
            result_timestamp = timestamp;
        }
    }

    return result;
}

int log_handler_file::get_oldest_file_index()
{
    int result = -1;
    std::filesystem::file_time_type result_timestamp;

    for (int i = 0; i < m_existing_files.size(); i++)
    {
        std::filesystem::file_time_type timestamp = std::filesystem::last_write_time(m_existing_files[i]);
        if (i == 0 || timestamp < result_timestamp)
        {
            result = i;
            result_timestamp = timestamp;
        }
    }

    return result;
}

void log_handler_file::write(log_level level, const std::string& message)
{
    if (!m_active)
    {
        return;
    }

    // Write out to current file.
    if (m_current_file)
    {
        size_t bytes_written = fwrite(message.data(), 1, message.size(), m_current_file);
        if (bytes_written != message.size())
        {
            // Failed to write data, lets close stream and ignore. Don't write a log here
            // or you are going to cause some recursion :P
            fclose(m_current_file);
            m_current_file = nullptr;
        }
    }

    // We may need to rotate now.
    if (m_current_file == nullptr || message.size() > m_current_file_remaining_space)
    {
        if (m_current_file)
        {
            fflush(m_current_file);
        }

        if (get_target_file(true) != m_current_file_path)
        {
            open_target_file(true);
        }
    }
    else
    {
        m_current_file_remaining_space -= message.size();
    }

    // Flush the stream periodically so data isn't lost.
    if (message.size() > m_bytes_till_flush)
    {
        if (m_current_file)
        {
            fflush(m_current_file);
        }

        m_bytes_till_flush = k_flush_interval_bytes;
    }
    else
    {
        m_bytes_till_flush -= message.size();
    }
}

}; // namespace workshop
