// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/app/app.h"
#include "workshop.core/drawing/pixmap.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.render_interface/ri_interface.h"

#include <unordered_map>

namespace ws {

class engine;
class asset_database;
class asset_manager;
class ri_texture;

// ================================================================================================
//  Represents meta-data that describes about a given asset - type/thumbnails/etc.
// ================================================================================================
class asset_database_metadata
{
public:

    // Path to the asset in the VFS.
    std::string path;

    // Gets the type of the asset as described in the yaml header.
    // These are defined in the k_asset_descriptor_type of each asset loader.
    std::string descriptor_type;

};

// ================================================================================================
//  Represents an entry within the asset database.
// ================================================================================================
class asset_database_entry
{
public:

    asset_database_entry(asset_database* database, asset_database_entry* parent);
    ~asset_database_entry();

    // Returns true if this entry is a directory.
    bool is_directory();

    // Returns true if this entry is a file.
    bool is_file();

    // Gets the full path to this entry.
    const char* get_path();

    // Gets a string that should be used for doing text-filtering on entries.
    // This is basically the path converted to lowercase.
    const char* get_filter_key();

    // Gets the filename of this entry.
    const char* get_name();

    // Returns true if the entry metadata has been fully loaded.
    bool has_metadata();

    // Returns metadata that describes the entry (type/thumbnails/etc).
    asset_database_metadata* get_metadata();

    // Gets a list of directories contained in this entry.
    // Valid only if a directory entry.
    std::vector<asset_database_entry*> get_directories();
    asset_database_entry* get_directory(const char* name, bool can_create = false);

    // Gets a list of files contained in this entry. 
    // Valid only if a directory entry.
    std::vector<asset_database_entry*> get_files();
    asset_database_entry* get_file(const char* name, bool can_create = false);

protected:
    void update_if_needed();

    void set_metadata(std::unique_ptr<asset_database_metadata>&& metadata);

private:
    friend class asset_database;

    asset_database* m_database;
    asset_database_entry* m_parent;

    std::unique_ptr<virtual_file_system_watcher> m_watcher;

    bool m_is_directory = false;
    
    bool m_has_queried_children = false;
    bool m_update_in_progress = false;

    std::string m_name;
    std::string m_path;
    std::string m_filter_key;

    std::vector<std::unique_ptr<asset_database_entry>> m_directories;
    std::vector<std::unique_ptr<asset_database_entry>> m_files;

    std::unique_ptr<asset_database_metadata> m_metadata;

    bool m_metadata_query_in_progress = false;

};

// ================================================================================================
//  The asset database is responsible for keeping a cache of asset information and providing
//  an interface to query the information.
// 
//  This is mostly used in the editor to display thumbnails/metadata about assets in the browser.
// ================================================================================================
class asset_database
{
public:
    struct thumbnail
    {
    public:
        // Thumbnail render of the asset if one is available.
        std::unique_ptr<pixmap> thumbnail;

        // Render texture version of thumbnail.
        std::unique_ptr<ri_texture> thumbnail_texture;

    private:
        friend class asset_database;

        // Number of frames the thumbnail has not been used for.
        size_t unused_frames = 0;
    };

public:

    asset_database(asset_manager* ass_manager);
    ~asset_database();

    // Gets the entry represented by the given path, or null if it doesn't currently exist.
    asset_database_entry* get(const char* path);

    // Sets the render interface used for generating thumbnails of assets.
    void set_render_interface(ri_interface* value);
    
    // Gets the thumbnail for the given file metadata.
    thumbnail* get_thumbnail(asset_database_entry* entry);

    // Should be called once a frame, will purge out any thumbnails that have not been used for a while.
    void clean_cache();

private:
    friend class asset_database_entry;

    void update_directory(asset_database_entry* parent, const char* name);
    void update_file_metadata(asset_database_entry* file);
    void update_directory_metadata(asset_database_entry* file);

    asset_database_entry* get(asset_database_entry* parent, const char* path, const std::vector<std::string>& fragments);

    void gather_metadata(asset_database_entry* entry, bool get_thumbnail);
    void cancel_gather_metadata(asset_database_entry* entry);

    void generate_thumbnail(const char* path, asset_database_metadata& metadata);

    std::unique_ptr<asset_database_metadata> generate_metadata(const char* path, bool get_thumbnail);

private:

    // Size of thumbnail generated in pixels.
    static inline constexpr size_t k_thumbnail_size = 256;

    // How many frames before an unused thumbnail is purged.
    static inline constexpr size_t k_thumbnail_removal_frames = 30;

    // How large the thumbnail cache can get before we start to purge old thumbnails.
    static inline constexpr size_t k_thumbnail_capacity = 200;

    asset_database_entry m_root;

    asset_manager* m_asset_manager;

    std::mutex m_thumbnail_mutex;
    std::unordered_map<std::string, std::unique_ptr<thumbnail>> m_thumbnail_cache;

    std::mutex m_metadata_mutex;
    std::vector<asset_database_entry*> m_pending_metadata;

    std::mutex m_mutex;

    ri_interface* m_render_interface;

};

}; // namespace ws
