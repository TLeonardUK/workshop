// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/assets/caches/asset_cache_disk.h"

namespace ws {

asset_cache_disk::asset_cache_disk(const std::filesystem::path& root, bool read_only)
    : m_root(root)
    , m_read_only(read_only)
{
}

bool asset_cache_disk::get(const asset_cache_key& key, std::string& storage_path)
{
    std::filesystem::path path = get_path_from_key(key);

    return false;
}

bool asset_cache_disk::set(const asset_cache_key& key, const char* temporary_file)
{
    std::filesystem::path path = get_path_from_key(key);

    return false;
}

bool asset_cache_disk::is_read_only()
{
    return m_read_only;
}

std::filesystem::path asset_cache_disk::get_path_from_key(const asset_cache_key& key)
{
    std::filesystem::path path = m_root / to_string(key.platform);

    std::string hash_string = key.hash();

    // First n characters are seperate directories to avoid just one massive
    // directory with a billion entries in it.
    constexpr size_t k_seperation_directory_count = 3;
    db_assert(hash_string.size() > k_seperation_directory_count);

    for (size_t i = 0; i < k_seperation_directory_count; i++)
    {
        path = path / std::string(1, hash_string[i]);
    }

    path = path / hash_string;

    return path;
}

}; // namespace workshop
