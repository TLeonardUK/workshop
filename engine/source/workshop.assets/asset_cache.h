// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/debug/debug.h"
#include "workshop.core/platform/platform.h"

#include "workshop.core/filesystem/virtual_file_system_types.h"

#include <filesystem>
#include <string>

namespace ws {

// State of a given file in a cache key.
struct asset_cache_key_file
{
    std::string path;
    virtual_file_system_time_point modified_time;
};

// Data about an asset that is used to generate a unique
// cache key to access the specific asset.
struct asset_cache_key
{
    // Source asset file being cached.
    asset_cache_key_file source;

    // All files that this asset relies on to compile.
    std::vector<asset_cache_key_file> dependencies;

    // Latest version of compiled data format.
    size_t version;

    // The platform the asset is compiled for.
    platform_type platform;

    // The release profile of the platform being compiled for.
    config_type config;

    // Calculates a string representation of the key data.
    // This can be used to identify the asset in the underlying
    // cache storage.
    std::string hash() const;
};

// ================================================================================================
//  This class implements the base class for asset caches - which are areas on disk/network/etc 
//  where compiled assets can be stored to avoid recompiling them unneccessarily.
// 
//  The asset_manager class can hold multiple asset caches which will be searched in priority
//  order.
// 
//  This class is thread safe.
// ================================================================================================
class asset_cache
{
public:

    // Gets the path to a given asset based on the cache key. 
    // Returns true if the cache key exists.
    //
    // Note: The asset can be stored in any kind of storage, do not assume its
    //       a path to a local filesystem. The storage_path is openable using the 
    //       engines virtual file system.
    virtual bool get(const asset_cache_key& key, std::string& storage_path) = 0;

    // Copies the given file to the backing storage of the cache.
    // Returns true if successfully added to cache.
    virtual bool set(const asset_cache_key& key, const char* temporary_file) = 0;

    // Returns true if this cache should only be read from.
    virtual bool is_read_only() = 0;

};

}; // namespace workshop
