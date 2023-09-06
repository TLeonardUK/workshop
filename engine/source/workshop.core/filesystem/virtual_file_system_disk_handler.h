// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/filesystem/virtual_file_system_handler.h"
#include "workshop.core/filesystem/path_watcher.h"

namespace ws {

class virtual_file_system_disk_handler;

// ================================================================================================
//  Helper class that stores the state of a path watcher for a disk handler.
// ================================================================================================
class virtual_file_system_disk_watcher : public virtual_file_system_watcher
{
public:

    virtual ~virtual_file_system_disk_watcher();

public:

    virtual_file_system_disk_handler* m_handler;
    callback_t m_callback;
    std::string m_path;
    bool m_is_directory = false;

};

// ================================================================================================
//  This class is the base class for protocol handlers that can be registered to the 
//  virtual file system.
// ================================================================================================
class virtual_file_system_disk_handler : public virtual_file_system_handler
{
public:

    virtual_file_system_disk_handler(const std::string& root, bool read_only);

    virtual std::unique_ptr<stream> open(const char* path, bool for_writing) override;
    virtual virtual_file_system_path_type type(const char* path) override;
    virtual bool remove(const char* path) override;
    virtual bool rename(const char* source, const char* destination) override;
    virtual bool create_directory(const char* path) override;
    virtual bool modified_time(const char* path, virtual_file_system_time_point& timepoint) override;
    virtual std::vector<std::string> list(const char* path, virtual_file_system_path_type type, bool recursive) override;
    virtual std::unique_ptr<virtual_file_system_watcher> watch(const char* path, virtual_file_system_watcher::callback_t callback) override;
    virtual void raise_watch_events() override;
    virtual bool get_disk_location(const char* path, std::string& output_path) override;
    virtual bool get_vfs_location(const char* path, std::string& output_path) override;

protected:
    std::filesystem::path resolve_path(const char* path);

private:
    friend class virtual_file_system_disk_watcher;

    std::string m_root;
    bool m_read_only;

    std::recursive_mutex m_registered_watchers_mutex;
    std::vector<virtual_file_system_disk_watcher*> m_registered_watchers;

    std::unique_ptr<path_watcher> m_path_watcher;

};

}; // namespace workshop
