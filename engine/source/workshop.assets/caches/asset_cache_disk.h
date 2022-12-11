// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.assets/asset_cache.h"

namespace ws {

class virtual_file_system_handler;

// ================================================================================================
//  This class implements and asset cache that uses a directory on disk as backing storage.
// 
//  This class is thread safe.
// ================================================================================================
class asset_cache_disk : public asset_cache
{
public:

    asset_cache_disk(const std::string& storage_protocol, const std::string& access_protocol, bool read_only);

    virtual bool get(const asset_cache_key& key, std::string& storage_path) override;
    virtual bool set(const asset_cache_key& key, const char* temporary_file) override;
    virtual bool is_read_only() override;

protected:

    std::string get_path_from_key(const asset_cache_key& key);

    void update_handlers_for_path(const std::string& virtual_path, const std::string& disk_path);
     
private:

    std::string m_storage_protocol;
    std::string m_access_protocol;
    bool m_read_only;

    std::vector<virtual_file_system_handler*> m_access_handlers;

};

}; // namespace workshop
