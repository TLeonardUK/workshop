// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.assets/asset_manager.h"
#include "workshop.assets/asset_loader.h"
#include "workshop.assets/asset_importer.h"
#include "workshop.assets/asset_cache.h"
#include "workshop.assets/asset.h"
#include "workshop.core/debug/debug.h"
#include "workshop.core/async/async.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/memory/memory_tracker.h"

#include <algorithm>
#include <thread>
#include <future>
#include <array>

// The asset manager is multithreaded, its important to know how it behaves before 
// attempting to make changes to it.
//
// An asset is first requested via a call to request_asset, this returns an asset_ptr
// which can be used to check the current state of the asset.
// 
// When the asset is requested a call to request_load is made which marks the asset
// as wanting to be loaded and notifies a background thread (which runs do_work) 
// that an asset state has changed.
//
// When all reference to an asset are lost a call to request_unload is made that 
// will mark the asset as wanting to be unloaded and notify the background thread.
//
// The background thread wakes up whenever notified and looks at pending tasks, if
// there are less in-process operations (loads or unloads) it task a pending task
// and begins processing it (in process_asset). 
//
// Processing an asset involes essentially running a state machine to determine if
// the asset is in the state it wants to be in and if not it will call begin_load
// or begin_unload to start changing to the start it wants to be in.
//
// begin_load and begin_unload queue asynchronous operations which run in the 
// task_scheduler worker pool. Once they finish doing their task process_asset
// is called again incase its state has changed while the operation has been in progress.
// 
// If the task is now in the correct state the asset_manager is done with it until its
// next state change.
//
// All functions accessible to calling-code (requesting an asset, checking an asset state, etc)
// are expected to be thread-safe and callable from anywhere.

namespace ws {

namespace {

// Holds the current asset that is having load_dependencies call on it in the current thread.
static thread_local asset_state* g_tls_current_load_dependencies_asset = nullptr;

// Holds the current asset that is having post_load call on it in the current thread.
static thread_local asset_state* g_tls_current_post_load_asset = nullptr;

};

asset_manager::asset_manager(platform_type asset_platform, config_type asset_config)
    : m_asset_platform(asset_platform)
    , m_asset_config(asset_config)
{
    m_max_concurrent_ops = task_scheduler::get().get_worker_count(task_queue::loading);

    m_load_thread = std::thread([this]() {

        db_set_thread_name("Asset Manager Coordinator");    
        do_work();

    });
}

asset_manager::~asset_manager()
{
    {
        std::unique_lock lock(m_states_mutex);
        m_shutting_down = true;
        m_states_convar.notify_all();
    }

    m_load_thread.join();
    m_load_thread = std::thread();
}

platform_type asset_manager::get_asset_platform()
{
    return m_asset_platform;
}

config_type asset_manager::get_asset_config()
{
    return m_asset_config;
}

asset_loader* asset_manager::get_loader_for_type(std::type_index type)
{
    std::scoped_lock lock(m_loaders_mutex);
    
    auto iter = std::find_if(m_loaders.begin(), m_loaders.end(), [type](const registered_loader& handle){
        return handle.loader->get_type() == type;
    });

    if (iter != m_loaders.end())
    {
        return iter->loader.get();
    }

    return nullptr;
}

asset_manager::loader_id asset_manager::register_loader(std::unique_ptr<asset_loader>&& loader)
{
    std::scoped_lock lock(m_loaders_mutex);
    
    registered_loader handle;
    handle.loader = std::move(loader);
    handle.id = m_next_loader_id++;
    m_loaders.push_back(std::move(handle));

    return handle.id;
}

void asset_manager::unregister_loader(loader_id id)
{
    std::scoped_lock lock(m_loaders_mutex);

    auto iter = std::find_if(m_loaders.begin(), m_loaders.end(), [id](const registered_loader& handle){
        return handle.id == id;
    });

    if (iter != m_loaders.end())
    {
        m_loaders.erase(iter);
    }
}

asset_manager::importer_id asset_manager::register_importer(std::unique_ptr<asset_importer>&& loader)
{
    std::scoped_lock lock(m_importers_mutex);

    registered_importer handle;
    handle.importer = std::move(loader);
    handle.id = m_next_importer_id++;
    m_importers.push_back(std::move(handle));

    return handle.id;
}

void asset_manager::unregister_importer(importer_id id)
{
    std::scoped_lock lock(m_importers_mutex);

    auto iter = std::find_if(m_importers.begin(), m_importers.end(), [id](const registered_importer& handle) {
        return handle.id == id;
    });

    if (iter != m_importers.end())
    {
        m_importers.erase(iter);
    }
}

std::vector<asset_importer*> asset_manager::get_asset_importers()
{
    std::scoped_lock lock(m_importers_mutex);

    std::vector<asset_importer*> ret;

    for (auto& ptr : m_importers)
    {
        ret.push_back(ptr.importer.get());
    }

    return ret;
}

asset_importer* asset_manager::get_importer_for_extension(const char* extension)
{
    std::scoped_lock lock(m_importers_mutex);

    for (auto& ptr : m_importers)
    {
        std::vector<std::string> extensions = ptr.importer->get_supported_extensions();
        if (std::find_if(extensions.begin(), extensions.end(), [extension](const std::string& val) { return _stricmp(extension, val.c_str()) == 0; }) != extensions.end())
        {
            return ptr.importer.get();
        }
    }

    return nullptr;
}

asset_manager::cache_id asset_manager::register_cache(std::unique_ptr<asset_cache>&& cache)
{
    std::scoped_lock lock(m_cache_mutex);
    
    registered_cache handle;
    handle.cache = std::move(cache);
    handle.id = m_next_cache_id++;
    m_caches.push_back(std::move(handle));

    return handle.id;
}

void asset_manager::unregister_cache(cache_id id)
{
    std::scoped_lock lock(m_cache_mutex);

    auto iter = std::find_if(m_caches.begin(), m_caches.end(), [id](const registered_cache& handle){
        return handle.id == id;
    });

    if (iter != m_caches.end())
    {
        m_caches.erase(iter);
    }
}

asset_loader* asset_manager::get_loader_for_type(const std::type_info* id)
{
    std::scoped_lock lock(m_loaders_mutex);

    auto iter = std::find_if(m_loaders.begin(), m_loaders.end(), [id](const registered_loader& handle){
        return &handle.loader->get_type() == id;
    });

    if (iter != m_loaders.end())
    {
        return iter->loader.get();
    }

    return nullptr;
}

asset_loader* asset_manager::get_loader_for_descriptor_type(const char* type)
{
    std::scoped_lock lock(m_loaders_mutex);

    auto iter = std::find_if(m_loaders.begin(), m_loaders.end(), [type](const registered_loader& handle){
        return strcmp(handle.loader->get_descriptor_type(), type) == 0;
    });

    if (iter != m_loaders.end())
    {
        return iter->loader.get();
    }

    return nullptr;
}

void asset_manager::drain_queue()
{
    std::unique_lock lock(m_states_mutex);

    while (m_pending_queue.size() > 0 || m_outstanding_ops.load() > 0)
    {
        m_states_convar.wait(lock);
    }
}

void asset_manager::increment_ref(asset_state* state, bool state_lock_held)
{  
    size_t result = state->references.fetch_add(1);

    if (result == 0)
    {
        if (state_lock_held)
        {
            request_load_lockless(state);
        }
        else
        {
            request_load(state);
        } 
    }
}

void asset_manager::decrement_ref(asset_state* state, bool state_lock_held)
{
    size_t result = state->references.fetch_sub(1);

    if (result == 1)
    {
        if (state_lock_held)
        {
            request_unload_lockless(state);
        }
        else
        {
            request_unload(state);
        }
    }
}

asset_state* asset_manager::create_asset_state(const std::type_info& id, const char* path, int32_t priority, bool is_hot_reload)
{
    db_assert_message(g_tls_current_post_load_asset == nullptr, "Assets cannot be requested during a post_load. Use load_dependencies instead.");
    
    std::unique_lock lock(m_states_mutex);
    return create_asset_state_lockless(id, path, priority, is_hot_reload);
}

asset_state* asset_manager::create_asset_state_lockless(const std::type_info& id, const char* path, int32_t priority, bool is_hot_reload)
{
    asset_state* state = nullptr;

    //  If we are hot reloading always load the asset.
    if (!is_hot_reload)
    {
        // See if the asset already exists.
        auto iter = std::find_if(m_states.begin(), m_states.end(), [&path](asset_state* state) {
            return !state->is_for_hot_reload && (_stricmp(state->path.c_str(), path) == 0);
        });

        if (iter != m_states.end())
        {
            state = *iter;
            state->priority = priority;

            // If we've previously failed to load, try and reload as we are requesting the asset again.
            if (state->loading_state == asset_loading_state::failed)
            {
                state->loading_state = asset_loading_state::unloaded;
                request_load_lockless(state);
            }
        }
    }

    // Create a new state if one didn't already exist.
    if (state == nullptr)
    {
        state = new asset_state();
        state->default_asset = nullptr;
        state->loading_state = asset_loading_state::unloaded;
        state->priority = priority;
        state->references = 0;
        state->type_id = &id;
        state->path = path;
        state->is_for_hot_reload = is_hot_reload;

        asset_loader* loader = get_loader_for_type(state->type_id);
        if (loader != nullptr)
        {
            state->default_asset = loader->get_default_asset();
        }

        m_states.push_back(state);
    }

    // If we are loading this as a dependent asset, keep track of the references so
    // we don't mark our state as completed.
    asset_state* parent_state = g_tls_current_load_dependencies_asset;
    if (parent_state != nullptr)
    {
        auto dependent_iter = std::find(parent_state->dependencies.begin(), parent_state->dependencies.end(), state);
        if (dependent_iter == parent_state->dependencies.end())
        {
            parent_state->dependencies.push_back(state);
            state->depended_on_by.push_back(parent_state);

            // Parent holds a ref to child until its fully unloaded.
            increment_ref(state, true);
        }
    }

    // Increment the states reference to avoid anything happening between this being returned
    // and the asset_ptr being created.
    increment_ref(state, true);

    return state;
}

/*
void asset_manager::request_reload(asset_state* state)
{
    std::unique_lock lock(m_states_mutex);
    db_assert_message(!state->is_pending, "Requested reload on asset already pending state change.");
    db_assert_message(state->loading_state == asset_loading_state::loaded || state->loading_state == asset_loading_state::failed, "Requested reload on asset that is not loaded.");

    request_load_lockless(state);
}
*/

void asset_manager::request_load(asset_state* state)
{
    std::unique_lock lock(m_states_mutex);
    return request_load_lockless(state);
}

void asset_manager::request_unload(asset_state* state)
{
    std::unique_lock lock(m_states_mutex);
    return request_unload_lockless(state);
}

void asset_manager::request_load_lockless(asset_state* state)
{
    state->should_be_loaded = true;

    bool expected = false;
    if (state->is_pending.compare_exchange_weak(expected, true))
    {
        // Insert into queue in priority order.
        auto iter = std::lower_bound(m_pending_queue.begin(), m_pending_queue.end(), state->priority, [](const asset_state* info, size_t value)
        {
            return info->priority < value;
        });
        m_pending_queue.insert(iter, state);

        m_states_convar.notify_all();
    }
}

void asset_manager::request_unload_lockless(asset_state* state)
{
    state->should_be_loaded = false;

    bool expected = false;
    if (state->is_pending.compare_exchange_weak(expected, true))
    {
        // Insert into queue in priority order.
        auto iter = std::lower_bound(m_pending_queue.begin(), m_pending_queue.end(), state->priority, [](const asset_state* info, size_t value)
        {
            return info->priority < value;
        });
        m_pending_queue.insert(iter, state);

        m_states_convar.notify_all();
    }
}

void asset_manager::wait_for_load(const asset_state* state)
{
    std::unique_lock lock(m_states_mutex);

    while (true)
    {
        if (state->loading_state == asset_loading_state::loaded ||
            state->loading_state == asset_loading_state::failed)
        {
            return;
        }

        m_states_convar.wait(lock);
    }
}

void asset_manager::do_work()
{
    std::unique_lock lock(m_states_mutex);

    while (!m_shutting_down)
    {
        if (m_outstanding_ops.load() < m_max_concurrent_ops)
        {
            if (m_pending_queue.size() > 0)
            {
                asset_state* state = m_pending_queue.back();
                m_pending_queue.pop_back();

                state->is_pending = false;

                process_asset(state, false);

                continue;
            }
        }

        m_states_convar.wait(lock);
    }
}

void asset_manager::process_asset(asset_state* state, bool release_operation_reference)
{
    std::scoped_lock lock(state->process_mutex);

    // If we are at the end of an operation we reduce the operation count here while
    // under the process_mutex to avoid race conditions.
    if (release_operation_reference)
    {
        state->current_operations.fetch_sub(1);
        db_verbose(asset, "[%s] Reduced operation count to %zi", state->path.c_str(), state->current_operations.load());
    }

    // A different thread is already operating on this asset. We don't need to do anything
    // here as they will handle processing the asset when they finish.
    if (state->current_operations.load() > 0)
    {
        db_verbose(asset, "[%s] Skipping process as operation count is %zi", state->path.c_str(), state->current_operations.load());
        return;
    }

    switch (state->loading_state)
    {
    case asset_loading_state::loaded:
        {
            if (!state->should_be_loaded)
            {
                begin_unload(state);
            }
            break;
        }
    case asset_loading_state::unloaded:
        {
            if (state->should_be_loaded)
            {
                begin_load(state);
            }
            break;
        }
    case asset_loading_state::failed:
        {
            // We do nothing to failed assets, they sit in
            // this state and return a default asset if available.

            break;
        }
    case asset_loading_state::waiting_for_dependencies:
        {
            // Do nothing while waiting for dependencies, the assets we depend on will move us out of this state.
            break;
        }
    default:
        {
            db_assert(false);
            break;
        }
    }
}

void asset_manager::begin_load(asset_state* state)
{
    db_assert(state->loading_state == asset_loading_state::unloaded)
    set_load_state(state, asset_loading_state::loading);

    m_outstanding_ops.fetch_add(1);

    state->current_operations.fetch_add(1);

    async("Load Asset", task_queue::loading, [this, state]() mutable {
    
        do_load(state);

        {
            std::unique_lock lock(m_states_mutex);

            asset_loading_state new_state;
            if (state->instance)
            {
                if (are_dependencies_loaded(state))
                {
                    // TODO: Move this out of the lock, right now post-load is serialized.
                    if (!post_load_asset(state))
                    {
                        new_state = asset_loading_state::failed;
                    }
                    else
                    {
                        new_state = asset_loading_state::loaded;
                    }
                }
                else
                {
                    new_state = asset_loading_state::waiting_for_dependencies;
                }
            }
            else
            {
                new_state = asset_loading_state::failed;
            }

            set_load_state(state, new_state);

            // Let anything we depend on know that we have loaded.
            for (asset_state* parent : state->depended_on_by)
            {
                if (parent->loading_state == asset_loading_state::waiting_for_dependencies)
                {
                    check_dependency_load_state(parent);
                }
            }

            // Process the asset again incase the requested state
            // has changed during this process.
            process_asset(state, true);

            m_outstanding_ops.fetch_sub(1);
            m_states_convar.notify_all();
        }
    
    });
}

bool asset_manager::post_load_asset(asset_state* state)
{
    // Mark which asset is being post_load'd so we can handle things
    // differently in dependent assets are loaded during post_load.
    asset_state* old_state = g_tls_current_post_load_asset;
    g_tls_current_post_load_asset = state;

    bool success = false;
    if (state->instance->post_load())
    {
        success = true;
    }
    else
    {
        asset_loader* loader = get_loader_for_type(state->type_id);
        if (loader)
        {
            loader->unload(state->instance);
        }
        state->instance = nullptr;
    }

    g_tls_current_post_load_asset = old_state;

    return success;
}

void asset_manager::check_dependency_load_state(asset_state* state)
{
    db_assert(state->loading_state == asset_loading_state::waiting_for_dependencies);

    bool all_loaded = are_dependencies_loaded(state);

    if (all_loaded)
    {
        // TODO: Move this out of the lock, right now post-load is serialized.
        if (!post_load_asset(state))
        {
            set_load_state(state, asset_loading_state::failed);

        }
        else
        {
            set_load_state(state, asset_loading_state::loaded);
        }

        // Propogate the load upwards to any parents that depend on us.
        for (asset_state* parent : state->depended_on_by)
        {
            if (parent->loading_state == asset_loading_state::waiting_for_dependencies)
            {
                check_dependency_load_state(parent);
            }
        }

        // Process the asset again incase the requested state
        // has changed during this process.
        process_asset(state, false);

        m_states_convar.notify_all();
    }
}

void asset_manager::begin_unload(asset_state* state)
{
    db_assert(state->loading_state == asset_loading_state::loaded);
    set_load_state(state, asset_loading_state::unloading);

    m_outstanding_ops.fetch_add(1);

    state->current_operations.fetch_add(1);

    async("Unload Asset", task_queue::loading, [this, state]() {
    
        do_unload(state);

        {
            std::unique_lock lock(m_states_mutex);

            // This is the final unload, nuke the state completely.
            if (state->references == 0)
            {
                // Remove reference from all assets we depend on.
                for (asset_state* dependent : state->dependencies)
                {
                    auto iter = std::find(dependent->depended_on_by.begin(), dependent->depended_on_by.end(), state);
                    db_assert(iter != dependent->depended_on_by.end());

                    // Release the ref the parent gained on the child in create_asset_state.
                    decrement_ref(dependent, true);

                    dependent->depended_on_by.erase(iter);
                }

                state->dependencies.clear();

                // Nuke the state.
                delete_state(state);
            }
            else
            {
                set_load_state(state, asset_loading_state::unloaded);

                // Process the asset again incase the requested state
                // has changed during this process.
                process_asset(state, true);
            }

            m_outstanding_ops.fetch_sub(1);
            m_states_convar.notify_all();
        }
    
    });
}

void asset_manager::delete_state(asset_state* state)
{
    if (state->hot_reload_state)
    {
        decrement_ref(state->hot_reload_state, true);
        state->hot_reload_state = nullptr;
    }

    if (auto iter = std::find(m_hot_reload_queue.begin(), m_hot_reload_queue.end(), state); iter != m_hot_reload_queue.end())
    {
        m_hot_reload_queue.erase(iter);
    }
    m_states.erase(std::find(m_states.begin(), m_states.end(), state));
    delete state;
}

bool asset_manager::search_cache_for_key(const asset_cache_key& cache_key, std::string& compiled_path)
{
    std::scoped_lock lock(m_cache_mutex);

    for (size_t i = 0; i < m_caches.size(); i++)
    {
        registered_cache& cache = m_caches[i];
        if (cache.cache->get(cache_key, compiled_path))
        {
            // If we have found it in a later cache, move it to earlier caches.
            // We assume later caches have higher latency/costs to access.
            if (i > 0)
            {
                size_t best_cache = std::numeric_limits<size_t>::max();

                for (size_t j = 0; j < i; i++)
                {
                    registered_cache& other_cache = m_caches[j];
                    if (!other_cache.cache->is_read_only())
                    {
                        db_verbose(asset, "[%s] Migrating compiled asset to earlier cache.", cache_key.source.path.c_str());
                        if (other_cache.cache->set(cache_key, compiled_path.c_str()))
                        {
                            best_cache = j;
                        }
                    }
                }

                // Now that we've populated all the caches, try to get the new path from the best cache.
                if (best_cache != std::numeric_limits<size_t>::max())
                {
                    m_caches[best_cache].cache->get(cache_key, compiled_path);
                }
            }

            return true;
        }
    }

    return false;
}

bool asset_manager::compile_asset(asset_cache_key& cache_key, asset_loader* loader, asset_state* state, std::string& compiled_path)
{
    std::string temporary_path = string_format("temp:%s", to_string(guid::generate()).c_str());
    
    asset_flags flags = asset_flags::none;
    if (state->is_for_hot_reload)
    {
        flags = static_cast<asset_flags>(static_cast<size_t>(flags) | static_cast<size_t>(asset_flags::hot_reload));
    }

    if (!loader->compile(state->path.c_str(), temporary_path.c_str(), m_asset_platform, m_asset_config, flags))
    {
        db_error(asset, "[%s] Failed to compile asset.", state->path.c_str());
        return false;
    }
    else
    {
        db_log(asset, "[%s] Finished compiling, storing in cache.", state->path.c_str());
    }

    // Store compiled version in all writable caches.
    {
        std::scoped_lock lock(m_cache_mutex);

        for (registered_cache& cache : m_caches)
        {
            if (!cache.cache->is_read_only())
            {
                db_verbose(asset, "[%s] Inserting compiled asset into cache.", state->path.c_str());

                if (cache.cache->set(cache_key, temporary_path.c_str()))
                {
                    if (compiled_path.empty())
                    {
                        cache.cache->get(cache_key, compiled_path);
                    }
                }
            }
        }

        // If we couldn't store it in the cache, use the temporary file directly.
        if (compiled_path.empty())
        {
            compiled_path = temporary_path;
        }

        // Otherwise we can remove the temporary file now.
        else
        {
            virtual_file_system::get().remove(temporary_path.c_str());
        }
    }

    return true;
}

bool asset_manager::get_asset_compiled_path(asset_loader* loader, asset_state* state, std::string& compiled_path)
{
    asset_cache_key cache_key;

    asset_flags flags = asset_flags::none;
    if (state->is_for_hot_reload)
    {
        flags = static_cast<asset_flags>(static_cast<size_t>(flags) | static_cast<size_t>(asset_flags::hot_reload));
    }

    std::string with_compiled_extension = state->path + k_compiled_asset_extension;

    // Try and find compiled version of asset in VFS. 
    if (virtual_file_system::get().type(with_compiled_extension.c_str()) == virtual_file_system_path_type::file)
    {
        compiled_path = with_compiled_extension;
    }

    // Otherwise check in cache for compiled version.
    else if (!m_caches.empty())
    {
        // Generate a key with no dependencies.
        if (!loader->get_cache_key(state->path.c_str(), m_asset_platform, m_asset_config, flags, cache_key, { }))
        {
            db_error(asset, "[%s] Failed to calculate cache key for asset.", state->path.c_str());
            return false;
        }

        // Search for key with no dependencies in cache.
        search_cache_for_key(cache_key, compiled_path);

        bool needs_compile = false;

        // Always compile if no compiled asset is found.
        if (compiled_path.empty())
        {
            db_log(asset, "[%s] No compiled version available, compiling now.", state->path.c_str());
            needs_compile = true;
        }
        // If compile asset exists, read the dependency header block and generate a cache key from the data
        // if it differs from the one in the header, we need to rebuild it.
        else
        {
            compiled_asset_header header;

            if (!loader->load_header(compiled_path.c_str(), header))
            {
                db_error(asset, "[%s] Failed to read header from compiled asset: %s", state->path.c_str(), compiled_path.c_str());
                return false;
            }

            asset_cache_key compiled_cache_key;
            if (!loader->get_cache_key(state->path.c_str(), m_asset_platform, m_asset_config, flags, compiled_cache_key, header.dependencies))
            {
                // This can fail if one of our dependencies has been deleted, in which case we know we need to rebuild.
                db_warning(asset, "[%s] Failed to calculate dependency cache key for asset, recompile required.", state->path.c_str());
                needs_compile = true;
            }
            else
            {
                state->cache_key = compiled_cache_key;

                if (compiled_cache_key.hash() != header.compiled_hash)
                {
                    db_warning(asset, "[%s] Compiled asset looks to be out of date, recompile required.", state->path.c_str());
                    needs_compile = true;
                }
            }
        }

        // If no compiled version is available, compile to a temporary location.
        if (needs_compile)
        {
            set_load_state(state, asset_loading_state::compiling);
            bool result = compile_asset(cache_key, loader, state, compiled_path);
            set_load_state(state, asset_loading_state::loading);

            if (!result)
            {
                return false;
            }
            
            // Run through this function again to grab the correct cache key.
            return get_asset_compiled_path(loader, state, compiled_path);
        }
    }

    return true;
}

void asset_manager::do_load(asset_state* state)
{
    asset_loader* loader = get_loader_for_type(state->type_id);
    if (loader != nullptr)
    {
        std::string compiled_path;

        if (!get_asset_compiled_path(loader, state, compiled_path))
        {
            db_error(asset, "[%s] Failed to determine compiled asset path.", state->path.c_str());
            return;
        }

        // Load the resulting compiled asset.
        if (!compiled_path.empty())
        {
            memory_scope scope(memory_type::asset, string_hash(state->path));

            state->instance = loader->load(compiled_path.c_str());

            if (state->instance)
            {
                // Mark which asset is being load_dependencies'd so we can handle things
                // differently in dependent assets are loaded during load_dependencies.
                asset_state* old_state = g_tls_current_load_dependencies_asset;
                g_tls_current_load_dependencies_asset = state;

                // Allow instance to load any dependent assets.
                if (!state->instance->load_dependencies())
                {
                    db_error(asset, "[%s] Failed to load asset dependencies.", state->path.c_str());

                    loader->unload(state->instance);
                    state->instance = nullptr;
                }

                g_tls_current_load_dependencies_asset = old_state;
            }
            else
            {
                db_error(asset, "[%s] Loader failed to load asset.", state->path.c_str());
            }
        }
        else
        {
            db_error(asset, "[%s] Failed to find compiled data for asset.", state->path.c_str());
        }
    }
    else
    {
        db_error(asset, "[%s] Failed to find loader for asset type.", state->path.c_str());
    }
}

void asset_manager::do_unload(asset_state* state)
{
    asset_loader* loader = get_loader_for_type(state->type_id);
    if (loader != nullptr)
    {
        loader->unload(state->instance);
        state->instance = nullptr;
    }
}

bool asset_manager::are_dependencies_loaded(asset_state* state)
{
    bool all_loaded = true;
    for (asset_state* child : state->dependencies)
    {
        if (child->loading_state != asset_loading_state::loaded && 
            child->loading_state != asset_loading_state::failed)
        {
            all_loaded = false;
        }
    }
    return all_loaded;
}

bool asset_manager::any_dependencies_failed(asset_state* state)
{
    for (asset_state* child : state->dependencies)
    {
        if (child->loading_state == asset_loading_state::failed)
        {
            return true;
        }
    }
    return false;
}

void asset_manager::set_load_state(asset_state* state, asset_loading_state new_state)
{
    //db_log(asset, "[%s] %s", state->path.c_str(), asset_loading_state_strings[static_cast<int>(new_state)]);
    asset_loading_state old_state = state->loading_state;
    
    state->loading_state = new_state;

    if (new_state == asset_loading_state::loading && old_state != asset_loading_state::compiling)
    {
        state->load_timer.start();
    }
    else if (new_state == asset_loading_state::loaded)
    {
        state->load_timer.stop();
        
        db_log(asset, "[%s] Loaded in %.2f ms", state->path.c_str(), state->load_timer.get_elapsed_ms());
    }

    if (!state->is_for_hot_reload)
    {
        if (new_state == asset_loading_state::loaded)
        {
            start_watching_for_reload(state);
        }
        else
        {
            stop_watching_for_reload(state);
        }
    }

    state->version.fetch_add(1);
    state->on_change_callback.broadcast();
}

void asset_manager::visit_assets(visit_callback_t callback)
{
    std::unique_lock lock(m_states_mutex);

    for (asset_state* state : m_states)
    {
        callback(state);
    }
}

void asset_manager::hot_reload(asset_state* state)
{
    std::unique_lock lock(m_states_mutex);

    if (state->loading_state != asset_loading_state::loaded)
    {
        db_log(core, "Not hot reloading, asset not loaded yet.");
        return;
    }

    if (state->hot_reload_state)
    {
        db_log(core, "Not hot reloading, already in hot reload queue: %s", state->path.c_str());
        return;
    }

    asset_loader* loader = get_loader_for_type(state->type_id);
    if (loader && !loader->can_hot_reload())
    {
        db_log(core, "Not hot reloading, asset type is not hot reloadable: %s", state->path.c_str());
        return;
    }

    state->hot_reload_state = create_asset_state_lockless(*state->type_id, state->path.c_str(), state->priority, true);

    // Keep state in memory while hot reloading it.
    increment_ref(state);

    db_log(core, "Queued asset for hot reload: %s", state->path.c_str());
    m_hot_reload_queue.push_back(state);
}

void asset_manager::start_watching_for_reload(asset_state* state)
{
    state->file_watchers.clear();

    auto callback = [this, state](const char* changed_path) {

        db_log(core, "Asset modified: %s (due to change to %s)", state->path.c_str(), changed_path);
        hot_reload(state);

    };

    // Create a path watcher for changes.
    state->file_watchers.push_back(virtual_file_system::get().watch(state->cache_key.source.path.c_str(), callback));
    for (asset_cache_key_file& file : state->cache_key.dependencies)
    {
        state->file_watchers.push_back(virtual_file_system::get().watch(file.path.c_str(), callback));
    }
}

void asset_manager::stop_watching_for_reload(asset_state* state)
{
    state->file_watchers.clear();
}

bool asset_manager::has_pending_hot_reloads()
{
    std::unique_lock lock(m_states_mutex);

    for (asset_state* state : m_hot_reload_queue)
    {
        if (state->hot_reload_state->loading_state == asset_loading_state::loaded ||
            state->hot_reload_state->loading_state == asset_loading_state::failed)
        {
            return true;
        }
    }
    
    return false;
}

size_t asset_manager::get_queue_size()
{
    std::unique_lock lock(m_states_mutex);

    return m_pending_queue.size() + m_outstanding_ops.load();
}

void asset_manager::apply_hot_reloads()
{
    std::unique_lock lock(m_states_mutex);

    for (auto iter = m_hot_reload_queue.begin(); iter != m_hot_reload_queue.end(); /* empty */)
    {
        asset_state* state = *iter;

        if (state->hot_reload_state->loading_state == asset_loading_state::failed)
        {
            db_log(core, "Failed to hot reload asset: %s", state->path.c_str());

            // Remove the temporary hot reload state.
            if (state->hot_reload_state)
            {
                decrement_ref(state->hot_reload_state, true);
                state->hot_reload_state = nullptr;
            }

            // Remove reference from state that we added when we queued it for hot reload.
            decrement_ref(state, true);

            iter = m_hot_reload_queue.erase(iter);
        }
        else if (state->hot_reload_state->loading_state == asset_loading_state::loaded)
        {
            db_log(core, "Swapping hot reloaded asset: %s", state->path.c_str());

            // Ask the asset loader to swap the internal state of the asset being hot reloaded.
            asset_loader* loader = get_loader_for_type(state->type_id);
            if (loader)
            {
                loader->hot_reload(state->instance, state->hot_reload_state->instance);
            }

            // Remove the temporary hot reload state.
            if (state->hot_reload_state)
            {
                decrement_ref(state->hot_reload_state, true);
                state->hot_reload_state = nullptr;
            }

            // Invoke any change callbacks for reloaded state.
            state->version.fetch_add(1);
            state->on_change_callback.broadcast();

            // Remove reference from state that we added when we queued it for hot reload.
            decrement_ref(state, true);

            iter = m_hot_reload_queue.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}

}; // namespace workshop
