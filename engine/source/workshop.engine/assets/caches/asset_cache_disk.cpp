// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/assets/caches/asset_cache_disk.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/filesystem/virtual_file_system_redirect_handler.h"
#include "workshop.core/filesystem/disk_stream.h"
#include "workshop.core/hashing/guid.h"

namespace ws {

asset_cache_disk::asset_cache_disk(const std::string& storage_protocol, const std::string& access_protocol, bool read_only)
    : m_storage_protocol(storage_protocol)
    , m_access_protocol(access_protocol)
    , m_read_only(read_only)
{
    m_access_handlers = virtual_file_system::get().get_handlers(access_protocol);
    db_assert_message(!m_access_handlers.empty(), "Disk asset cache using access protocol that hasn't been registered to virtual file system.");
}

bool asset_cache_disk::get(const asset_cache_key& key, std::string& storage_path)
{
    virtual_file_system& vfs = virtual_file_system::get();

    std::string path = get_path_from_key(key);

    if (vfs.type(path.c_str()) == virtual_file_system_path_type::file)
    {
        update_handlers_for_path(key.source.path, path);

        storage_path = virtual_file_system::replace_protocol(key.source.path.c_str(), m_access_protocol.c_str());

        return true;
    }

    return false;
}

bool asset_cache_disk::set(const asset_cache_key& key, const char* temporary_file)
{
    virtual_file_system& vfs = virtual_file_system::get();

    std::string path = get_path_from_key(key);

    // Copy over to the cache with a temporary extension to avoid anything trying to read it while 
    // its being copied over.
    std::string tmp_cache_path = path + string_format(".tmp_%s", to_string(guid::generate()).c_str());
    std::string tmp_cache_dir = virtual_file_system::get_parent(tmp_cache_path.c_str());

    if (!vfs.exists(tmp_cache_dir.c_str()))
    {
        if (!vfs.create_directory(tmp_cache_dir.c_str()))
        {
            db_error(asset, "[%s] Failed to create directories in cache: %s", temporary_file, tmp_cache_dir.c_str());
            return false;
        }
    }

    // Check if already cached.
    if (vfs.exists(path.c_str()))
    {
        // Retry several times before failing, if this is a remote drive other 
        // clients may be trying to access the file.
        for (size_t attempt = 0; vfs.exists(path.c_str()); attempt++)
        {
            if (attempt > 30)
            {
                db_error(asset, "[%s] Failed to delete existing cache file to make way for new file: %s", temporary_file, path.c_str());
                return false;
            }

            if (vfs.remove(path.c_str()))
            {
                break;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    if (!vfs.copy(temporary_file, tmp_cache_path.c_str()))
    {
        db_error(asset, "[%s] Failed to copy data to destination file: %s", temporary_file, tmp_cache_dir.c_str());
        return false;
    }

    // Now rename to final filename atomically.
    if (!vfs.rename(tmp_cache_path.c_str(), path.c_str()))
    {
        db_error(asset, "[%s] Failed to copy data to destination file: %s", temporary_file, tmp_cache_dir.c_str());
        return false;
    }

    update_handlers_for_path(key.source.path, path);

    return true;
}

bool asset_cache_disk::is_read_only()
{
    return m_read_only;
}

std::string asset_cache_disk::get_path_from_key(const asset_cache_key& key)
{
    std::string path = m_storage_protocol + ":" + to_string(key.platform);

    std::string hash_string = key.hash();

    // First n characters are seperate directories to avoid just one massive
    // directory with a billion entries in it.
    constexpr size_t k_seperation_directory_count = 3;
    db_assert(hash_string.size() > k_seperation_directory_count);

    for (size_t i = 0; i < k_seperation_directory_count; i++)
    {
        path += "/" + std::string(1, hash_string[i]);
    }

    path += "/" + hash_string;

    return path;
}

void asset_cache_disk::update_handlers_for_path(const std::string& virtual_path, const std::string& disk_path)
{
    std::string protocol, filename;
    virtual_file_system::crack(virtual_path.c_str(), protocol, filename);

    for (virtual_file_system_handler* handler : m_access_handlers)
    {
        virtual_file_system_redirect_handler* aliased_handler = dynamic_cast<virtual_file_system_redirect_handler*>(handler);
        if (aliased_handler)
        {
            aliased_handler->alias(filename.c_str(), disk_path.c_str());
        }
    }
}

}; // namespace workshop
