// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/assets/caches/asset_cache_disk.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/filesystem/virtual_file_system_aliased_disk_handler.h"
#include "workshop.core/filesystem/disk_stream.h"
#include "workshop.core/hashing/guid.h"

namespace ws {

// TODO: Make this read/write/etc from the vfs rather than directly via std::filesystem, we can mount the cache folder
//       as a protocol.
// TODO: "cache" protocol should look into asset caches, not asset caches populated cache protocol.

asset_cache_disk::asset_cache_disk(const std::string& protocol, const std::filesystem::path& root, bool read_only)
    : m_protocol(protocol)
    , m_root(root)
    , m_read_only(read_only)
{
    m_handlers = virtual_file_system::get().get_handlers(protocol);
    db_assert_message(!m_handlers.empty(), "Disk asset cache using protocol that hasn't been registered to virtual file system.");
}

bool asset_cache_disk::get(const asset_cache_key& key, std::string& storage_path)
{
    std::filesystem::path path = get_path_from_key(key);

    if (std::filesystem::exists(path))
    {
        update_handlers_for_path(key.source.path, path);

        storage_path = virtual_file_system::replace_protocol(key.source.path.c_str(), m_protocol.c_str());

        return true;
    }

    return false;
}

bool asset_cache_disk::set(const asset_cache_key& key, const char* temporary_file)
{
    std::filesystem::path path = get_path_from_key(key);

    // Copy over to the cache with a temporary extension to avoid anything trying to read it while 
    // its being copied over.
    std::filesystem::path tmp_cache_path = path.string() + string_format(".tmp_%s", to_string(guid::generate()).c_str());
    std::filesystem::path tmp_cache_dir = tmp_cache_path.parent_path();

    if (!std::filesystem::exists(tmp_cache_dir))
    {
        if (!std::filesystem::create_directories(tmp_cache_dir))
        {
            db_error(asset, "[%s] Failed to create directories in cache: %s", temporary_file, tmp_cache_dir.string().c_str());
            return false;
        }
    }

    // Check if already cached.
    if (std::filesystem::exists(path))
    {
        // Retry several times before failing, if this is a remote drive other 
        // clients may be trying to access the file.
        for (size_t attempt = 0; std::filesystem::exists(path); attempt++)
        {
            if (attempt > 30)
            {
                db_error(asset, "[%s] Failed to delete existing cache file to make way for new file: %s", temporary_file, path.string().c_str());
                return false;
            }

            try
            {
                if (std::filesystem::remove(path))
                {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            catch (std::filesystem::filesystem_error)
            {
                // Ignore.
            }
        }
    }

    // If path is to part of the VFS, copy the data from there.
    if (virtual_file_system::get().type(temporary_file) == virtual_file_system_path_type::file)
    {
        // TODO: Wrap in virtual_file_system::copy_file()

        std::unique_ptr<stream> source = virtual_file_system::get().open(temporary_file, false); 
        std::unique_ptr<disk_stream> dest = std::make_unique<disk_stream>();
        if (!dest->open(tmp_cache_path, true))
        {
            db_error(asset, "[%s] Failed to open destination file: %s", temporary_file, tmp_cache_dir.string().c_str());
            return false;
        }
        if (!source->copy_to(*dest))
        {
            db_error(asset, "[%s] Failed to copy data to destination file: %s", temporary_file, tmp_cache_dir.string().c_str());
            return false;
        }
    }
    // Otherwise we assume its a raw path.
    else
    {
        bool success = false;
        try
        {
            success = std::filesystem::copy_file(temporary_file, tmp_cache_path);
        }
        catch (std::filesystem::filesystem_error)
        {
            // Ignore.
        }

        if (!success)
        {
            db_error(asset, "[%s] Failed to copy temporary file to cache: %s", temporary_file, tmp_cache_path.string().c_str());
            return false;
        }
    }
    
    // Now rename to final filename atomically.
    std::filesystem::rename(tmp_cache_path, path);

    update_handlers_for_path(key.source.path, path);

    return true;
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

void asset_cache_disk::update_handlers_for_path(const std::string& virtual_path, const std::filesystem::path& disk_path)
{
    std::string protocol, filename;
    virtual_file_system::crack(virtual_path.c_str(), protocol, filename);

    for (virtual_file_system_handler* handler : m_handlers)
    {
        virtual_file_system_aliased_disk_handler* aliased_handler = dynamic_cast<virtual_file_system_aliased_disk_handler*>(handler);
        if (aliased_handler)
        {
            aliased_handler->alias(filename.c_str(), disk_path);
        }
    }
}

}; // namespace workshop
