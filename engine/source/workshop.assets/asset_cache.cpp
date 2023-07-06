// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.assets/asset_cache.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.core/hashing/hash.h"

#include <filesystem>

namespace ws {

std::string asset_cache_key::hash() const
{
    std::string result;

    // Generate hash from key source material.
    size_t hash = 0;
    hash_combine(hash, source.path);
    hash_combine(hash, source.modified_time.time_since_epoch().count());
    for (const asset_cache_key_file& dep : dependencies)
    {
        hash_combine(hash, dep.path);
        hash_combine(hash, dep.modified_time.time_since_epoch().count());
    }
    hash_combine(hash, version);
    hash_combine(hash, static_cast<size_t>(platform));
    hash_combine(hash, static_cast<size_t>(config));
    hash_combine(hash, static_cast<size_t>(flags));
    result.append(std::to_string(hash));

    // We append the filename of the asset to try and avoid any hash collisions.
    std::string filename = string_filter_out(source.path, "\\/:", '_');
    std::string compiled_filename = filename.append(asset_manager::k_compiled_asset_extension);
    result.append("_" + compiled_filename);

    return result;
}

}; // namespace workshop
