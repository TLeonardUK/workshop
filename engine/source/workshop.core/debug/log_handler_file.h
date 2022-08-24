// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/debug/log_handler.h"

#include <filesystem>

namespace ws {

// ================================================================================================
//  Log handler that writes to a rotating set of log files in a given directory.
// ================================================================================================
struct log_handler_file 
    : public log_handler
{
public:

    log_handler_file(const std::filesystem::path& root_directory, size_t file_count, size_t max_file_size);
    
    virtual void write(log_level level, const std::string& message) override;

protected:

    // Gets the best file available that we should write to. This can return
    // a path to a file that doesn't exist if we rotate to a new file.
    std::filesystem::path get_target_file(bool no_append);

    // Opens the target file as the current one for writing.
    void open_target_file(bool no_append);

    // Removes old files to keep us under the max file limit.
    void cull_old_files();

    // Gets the file with the newest modified time in m_existing_files.
    int get_newest_file_index();

    // Gets the file with the oldest modified time in m_existing_files.
    int get_oldest_file_index();

private:

    inline static const std::filesystem::path k_log_extension = ".log";
    inline static const size_t k_flush_interval_bytes = 2 * 1024;

    std::vector<std::filesystem::path> m_existing_files;

    std::filesystem::path m_current_file_path;
    FILE* m_current_file = nullptr;
    size_t m_current_file_remaining_space = 0;

    size_t m_bytes_till_flush = 0;

    std::filesystem::path m_file_directory;
    size_t m_file_count;
    size_t m_max_file_size;

    bool m_active = false;

};

}; // namespace workshop
