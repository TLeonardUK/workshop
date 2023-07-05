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
//  The redirect handle is intercepts calls to its protocol and redirects the calls to 
//  other manually-registered paths
// ================================================================================================
class virtual_file_system_redirect_handler : public virtual_file_system_handler
{
public:

    virtual_file_system_redirect_handler(bool read_only);

    virtual std::unique_ptr<stream> open(const char* path, bool for_writing) override;
    virtual virtual_file_system_path_type type(const char* path) override;
    virtual bool remove(const char* path) override;
    virtual bool rename(const char* source, const char* destination) override;
    virtual bool create_directory(const char* path) override;
    virtual bool modified_time(const char* path, virtual_file_system_time_point& timepoint) override;
    virtual std::vector<std::string> list(const char* path, virtual_file_system_path_type type) override;
    virtual std::unique_ptr<virtual_file_system_watcher> watch(const char* path, virtual_file_system_watcher::callback_t callback) override;
    virtual void raise_watch_events() override;

    void alias(const char* virtual_path, const char* target_path);

protected:

    bool get_target_path(const char* virtual_path, std::string& target_path);

private:
    std::shared_mutex m_alias_mutex;
    std::unordered_map<std::string, std::string> m_aliases;

    bool m_read_only;

};

}; // namespace workshop
