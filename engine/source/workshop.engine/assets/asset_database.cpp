// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/assets/asset_database.h"
#include "workshop.core/containers/string.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/async/async.h"
#include "workshop.core/async/task_scheduler.h"
#include "workshop.assets/asset_loader.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.render_interface/ri_texture_compiler.h"

namespace ws {
namespace {

std::string join_path(const char* a, const char* b)
{
    if (strlen(a) == 0)
    {
        return b;
    }

    std::string a_str = a;
    if (a_str[a_str.length() - 1] == ':')
    {
        return string_format("%s%s", a, b);
    }

    return string_format("%s/%s", a, b);
}

};

asset_database_entry::asset_database_entry(asset_database* database, asset_database_entry* parent)
    : m_database(database)
    , m_parent(parent)
{
}

asset_database_entry::~asset_database_entry()
{
    if (m_metadata_query_in_progress)
    {
        m_database->cancel_gather_metadata(this);
        m_metadata_query_in_progress = false;
    }
}

bool asset_database_entry::is_directory()
{
    return m_is_directory;
}

bool asset_database_entry::is_file()
{
    return !m_is_directory;
}

const char* asset_database_entry::get_path()
{
    return m_path.c_str();
}

const char* asset_database_entry::get_filter_key()
{
    return m_filter_key.c_str();
}

const char* asset_database_entry::get_name()
{
    return m_name.c_str();
}

void asset_database_entry::update_if_needed()
{
    if (!m_has_queried_children && m_parent && m_is_directory)
    {
        m_database->update_directory(m_parent, m_name.c_str());
    }
}

bool asset_database_entry::has_metadata()
{
    return m_metadata != nullptr;
}

asset_database_metadata* asset_database_entry::get_metadata()
{
    return m_metadata.get();
}

void asset_database_entry::set_metadata(std::unique_ptr<asset_database_metadata>&& metadata)
{
    m_metadata = std::move(metadata);
    m_metadata_query_in_progress = false;
}

std::vector<asset_database_entry*> asset_database_entry::get_directories()
{
    update_if_needed();

    std::vector<asset_database_entry*> ret;
    for (auto& ptr : m_directories)
    {
        asset_database_entry* entry = ptr.get();
        //entry->update_if_needed();

        ret.push_back(entry);
    }

    return ret;
}

std::vector<asset_database_entry*> asset_database_entry::get_files()
{
    update_if_needed();

    std::vector<asset_database_entry*> ret;
    for (auto& ptr : m_files)
    {
        asset_database_entry* entry = ptr.get();
        entry->update_if_needed();

        ret.push_back(entry);
    }

    return ret;
}

asset_database_entry* asset_database_entry::get_directory(const char* name, bool can_create)
{
    update_if_needed();

    auto iter = std::find_if(m_directories.begin(), m_directories.end(), [name](auto& entry) {
        return _stricmp(entry->m_name.c_str(), name) == 0;
    });
    
    if (iter != m_directories.end())
    {
        return (*iter).get();
    }

    if (can_create)
    {
        std::unique_ptr<asset_database_entry> entry = std::make_unique<asset_database_entry>(m_database, this);
        entry->m_is_directory = true;
        entry->m_path = join_path(m_path.c_str(), name);
        entry->m_filter_key = string_lower(entry->m_path);
        entry->m_name = name;

        asset_database_entry* ret = entry.get();
        m_directories.push_back(std::move(entry));

        return ret;
    }

    return nullptr;
}

asset_database_entry* asset_database_entry::get_file(const char* name, bool can_create)
{
    update_if_needed();

    auto iter = std::find_if(m_files.begin(), m_files.end(), [name](auto& entry) {
        return _stricmp(entry->m_name.c_str(), name) == 0;
    });

    if (iter != m_files.end())
    {
        return (*iter).get();
    }

    if (can_create)
    {
        std::unique_ptr<asset_database_entry> entry = std::make_unique<asset_database_entry>(m_database, this);
        entry->m_is_directory = false;
        entry->m_path = join_path(m_path.c_str(), name);
        entry->m_filter_key = string_lower(entry->m_path);
        entry->m_name = name;

        asset_database_entry* ret = entry.get();
        m_files.push_back(std::move(entry));

        return ret;
    }

    return nullptr;
}

asset_database::asset_database(asset_manager* ass_manager)
    : m_root(this, nullptr)
    , m_asset_manager(ass_manager)
{
}

asset_database::~asset_database()
{
}

void asset_database::set_render_interface(ri_interface* value)
{
    m_render_interface = value;
}

void asset_database::update_file_metadata(asset_database_entry* file)
{
    if (virtual_file_system::get_extension(file->get_path()) == asset_manager::k_asset_extension)
    {
        file->m_metadata_query_in_progress = true;
        gather_metadata(file, false);
    }
}

void asset_database::update_directory_metadata(asset_database_entry* file)
{
    // Nothing yet: Get thumbnails/types/etc.
}

void asset_database::update_directory(asset_database_entry* parent, const char* name)
{
    std::string full_path = join_path(parent->m_path.c_str(), name);
    
    // We don't check for existance of the first fragment as thats going to be the protocol.
    if (parent->m_path.empty() && !virtual_file_system::get().exists(full_path.c_str()))
    {
        return;
    }

    asset_database_entry* directory = parent->get_directory(name, true);
    
    if (directory->m_update_in_progress)
    {
        return;
    }
    directory->m_update_in_progress = true;
    directory->m_has_queried_children = true;

    // ------------------------------------------------------------------------------------------
    // Handle directory reconcile
    // ------------------------------------------------------------------------------------------

    std::vector<std::string> child_dirs = virtual_file_system::get().list(full_path.c_str(), virtual_file_system_path_type::directory, true, false);

    // Erase directories that no longer exist.
    for (auto iter = directory->m_directories.begin(); iter != directory->m_directories.end(); /* empty */)
    {
        auto child_iter = std::find_if(child_dirs.begin(), child_dirs.end(), [iter](auto& val) {
            return _stricmp(val.c_str(), (*iter)->m_name.c_str()) == 0;
        });

        if (child_iter == child_dirs.end())
        {
            iter = directory->m_directories.erase(iter);
        }
        else
        {
            iter++;
        }
    }

    // Update/Add existing directories.
    for (const std::string& path : child_dirs)
    {
        asset_database_entry* entry =  directory->get_directory(path.c_str(), true);
        update_directory_metadata(entry);
    }

    // ------------------------------------------------------------------------------------------
    // Handle file reconcile
    // ------------------------------------------------------------------------------------------

    std::vector<std::string> child_files = virtual_file_system::get().list(full_path.c_str(), virtual_file_system_path_type::file, true, false);

    // Erase directories that no longer exist.
    for (auto iter = directory->m_files.begin(); iter != directory->m_files.end(); /* empty */)
    {
        auto child_iter = std::find_if(child_files.begin(), child_files.end(), [iter](auto& val) {
            return _stricmp(val.c_str(), (*iter)->m_name.c_str()) == 0;
        });

        if (child_iter == child_files.end())
        {
            iter = directory->m_files.erase(iter);
        }
        else
        {
            iter++;
        }
    }

    // Update/Add existing directories.
    for (const std::string& path : child_files)
    {
        asset_database_entry* entry =  directory->get_file(path.c_str(), true);
        update_file_metadata(entry);
    }

    directory->m_update_in_progress = false;

    // Queue up a path watcher to update directory if any changes are needed.
    if (!directory->m_watcher)
    {
        directory->m_watcher = virtual_file_system::get().watch(full_path.c_str(), [directory, full_path](const char* changed_path) {
            directory->m_has_queried_children = false;
        });
    }
}

asset_database_entry* asset_database::get(asset_database_entry* parent, const char* path, const std::vector<std::string>& fragments)
{
    const std::string& frag = fragments.front();

    // This can occur if we query a root entry, eg. data:/
    if (frag.empty())
    {
        return parent;
    }

    // Look for a file or directory with the name of the remaining fragment.
    if (fragments.size() == 1)
    {
        if (asset_database_entry* entry = parent->get_directory(frag.c_str()))
        {
            return entry;
        }
        else if (asset_database_entry* entry = parent->get_file(frag.c_str()))
        {
            return entry;
        }
    }
    // Otherwise look for directory with the name of the remaining fragment.
    else
    {    
        asset_database_entry* entry = parent->get_directory(frag.c_str());

        if (!entry)
        {
            update_directory(parent, frag.c_str());
            entry = parent->get_directory(frag.c_str());
        }

        if (entry)
        {
            std::vector<std::string> remaining_fragments(fragments.begin() + 1, fragments.end());
            return get(entry, path, remaining_fragments);
        }
    }
    
    return nullptr;
}

asset_database_entry* asset_database::get(const char* path)
{
    std::vector<std::string> fragments = string_split(path, "/");
    if (fragments.empty())
    {
        return nullptr;
    }

    return get(&m_root, path, fragments);
}


void asset_database::gather_metadata(asset_database_entry* entry, bool get_thumbnail)
{
    {
        std::scoped_lock lock(m_metadata_mutex);

        // Already queued, ignore.
        auto iter = std::find(m_pending_metadata.begin(), m_pending_metadata.end(), entry);
        if (iter != m_pending_metadata.end())
        {
            return;
        }
        
        m_pending_metadata.push_back(entry);
    }

    std::string path = entry->get_path();

    async("Generate Metadata", task_queue::loading, [this, path, entry, get_thumbnail]() {

        db_verbose(engine, "Gathering metadata for %s", path.c_str());
        std::unique_ptr<asset_database_metadata> metadata = generate_metadata(path.c_str(), get_thumbnail);

        // If entry is still queued (eg. it hasn't been destroyed), remove it from the list
        // and set the metadata on it.
        {
            std::unique_lock lock(m_metadata_mutex);

            auto iter = std::find(m_pending_metadata.begin(), m_pending_metadata.end(), entry);
            if (iter != m_pending_metadata.end())
            {
                entry->set_metadata(std::move(metadata));
                m_pending_metadata.erase(iter);
            }
        }

    });
}

void asset_database::cancel_gather_metadata(asset_database_entry* entry)
{
    std::scoped_lock lock(m_metadata_mutex);

    auto iter = std::find(m_pending_metadata.begin(), m_pending_metadata.end(), entry);
    if (iter != m_pending_metadata.end())
    {
        m_pending_metadata.erase(iter);
    }
}

asset_database::thumbnail* asset_database::get_thumbnail(asset_database_entry* entry)
{
    std::scoped_lock lock(m_thumbnail_mutex);

    auto iter = m_thumbnail_cache.find(entry->m_path.c_str());
    if (iter != m_thumbnail_cache.end())
    {
        iter->second->unused_frames = 0;

        // An entry in the cache with a null pixmap is to keep 
        // track of thumbnails that fail to generate, rather than reloading them
        // constantly.
        if (!iter->second->thumbnail)
        {
            return nullptr;
        }

        return iter->second.get();
    }
    else
    {
        gather_metadata(entry, true);
    }

    return nullptr;
}

void asset_database::clean_cache()
{
    std::scoped_lock lock(m_thumbnail_mutex);

    for (auto iter = m_thumbnail_cache.begin(); iter != m_thumbnail_cache.end() && m_thumbnail_cache.size() > k_thumbnail_capacity; /* empty */)
    {
        iter->second->unused_frames++;
        if (iter->second->unused_frames > k_thumbnail_removal_frames)
        {
            iter = m_thumbnail_cache.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}

std::unique_ptr<asset_database_metadata> asset_database::generate_metadata(const char* path, bool get_thumbnail)
{
    std::unique_ptr<asset_database_metadata> ret = nullptr;

    // Parse the type information out of the source file.
    std::unique_ptr<stream> stream = virtual_file_system::get().open(path, false);
    if (!stream)
    {
        db_error(asset, "[%s] (Generating metadata) Failed to open stream to asset.", path);
        return ret;
    }

    std::string contents = stream->read_all_string();
    try
    {
        YAML::Node node = YAML::Load(contents);

        YAML::Node type_node = node["type"];

        if (!type_node.IsDefined() || type_node.Type() != YAML::NodeType::Scalar)
        {
            throw std::exception("type node is malformed.");
        }
        else
        {
            ret = std::make_unique<asset_database_metadata>();
            ret->descriptor_type = type_node.as<std::string>();
            ret->path = path;

            if (get_thumbnail)
            {
                generate_thumbnail(path, *ret);
            }
        }

    }
    catch (YAML::ParserException& exception)
    {
        db_error(asset, "[%s] (Generating metadata) Error parsing asset file: %s", path, exception.msg.c_str());
    }
    catch (std::exception& exception)
    {
        db_error(asset, "[%s] (Generating metadata)  Error loading asset file: %s", path, exception.what());
    }

    return ret;
}

void asset_database::generate_thumbnail(const char* path, asset_database_metadata& metadata)
{
    std::unique_ptr<thumbnail> thumb = std::make_unique<thumbnail>();

    // Ask loader to generate a thumbnail.
    asset_loader* loader = m_asset_manager->get_loader_for_descriptor_type(metadata.descriptor_type.c_str());
    if (loader)
    {
        // Grab the cache key of the asset to use for indexing into the thumbnail cache.
        asset_cache_key cache_key;
        if (!loader->get_cache_key(path, m_asset_manager->get_asset_platform(), m_asset_manager->get_asset_config(), asset_flags::none, cache_key, {}))
        {
            db_error(asset, "[%s] Failed to calculate cache key for asset.", path);
            return;
        }

        // Look in thumbnail cache for key.
        std::string cache_path = string_format("thumbnail-cache:%s.png", cache_key.hash().c_str());
        if (virtual_file_system::get().exists(cache_path.c_str()))
        {
            auto loaded_pixmaps = pixmap::load(cache_path.c_str());
            if (!loaded_pixmaps.empty())
            {
                thumb->thumbnail = std::move(loaded_pixmaps[0]);
            }
        }
        // If it doesn't exist in cache, regenerate it.
        else
        {
            thumb->thumbnail = loader->generate_thumbnail(path, k_thumbnail_size);
            if (thumb->thumbnail)
            {
                thumb->thumbnail->save(cache_path.c_str());
            }
        }

        // Generate the texture from the thumbnail pix.
        if (thumb->thumbnail)
        {
            ri_texture::create_params params;
            params.width = k_thumbnail_size;
            params.height = k_thumbnail_size;
            params.dimensions = ri_texture_dimension::texture_2d;
            params.format = ri_texture_format::R8G8B8A8;

            std::vector<ri_texture_compiler::texture_face> faces;
            ri_texture_compiler::texture_face& face = faces.emplace_back();
            face.mips.push_back(thumb->thumbnail.get());

            std::vector<uint8_t> texture_data;
            std::unique_ptr<ri_texture_compiler> compiler = m_render_interface->create_texture_compiler();
            compiler->compile(params.dimensions, params.width, params.height, 1, faces, texture_data);

            params.data = std::span<uint8_t>(texture_data.data(), texture_data.size());

            thumb->thumbnail_texture = m_render_interface->create_texture(params, string_format("Thumbnail: %s", path).c_str());
        }
    }

    // Insert resulting thumbnail into cache. Insert it regardless of if we produced a thumbnail or
    // not so we can avoid constantly trying to reload it.
    {
        std::scoped_lock lock(m_thumbnail_mutex);
        m_thumbnail_cache[metadata.path] = std::move(thumb);
    }
}

}; // namespace ws
