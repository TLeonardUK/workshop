// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/debug/debug.h"
#include "workshop.core/perf/timer.h"
#include "workshop.core/utils/event.h"
#include "workshop.assets/asset_loader.h"
#include "workshop.assets/asset_cache.h"

#include <thread>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace ws {

class asset;
class asset_manager;

// Loading state of an asset.
enum class asset_loading_state
{
    unloaded,
    unloading,
    loading,
    waiting_for_dependencies,
    loaded,
    failed,

    COUNT
};

// Internal state representing the current loading state of an asset.
struct asset_state
{
    std::atomic_size_t references = 0;
    std::atomic_bool is_pending = false;
    std::atomic_bool should_be_loaded = false;

    asset* instance = nullptr;

    std::string path = "";
    int32_t priority = 0;
    asset_loading_state loading_state = asset_loading_state::unloaded;
    asset* default_asset = nullptr;

    const std::type_info* type_id;

    std::vector<asset_state*> dependencies;
    std::vector<asset_state*> depended_on_by;

    event<> on_change_callback;

    timer load_timer;
};

// ================================================================================================
//  Represents a reference to a given asset.
// 
//  The asset is not guaranteed to be loaded, you can use the provided interface to determine
//  the loading state and optionally syncronously load.
// 
//  This class is thread safe.
// ================================================================================================
template <typename asset_type>
class asset_ptr
{
public:
    asset_ptr();
    asset_ptr(asset_manager* manager, asset_state* state);
    asset_ptr(const asset_ptr& other);
    asset_ptr(asset_ptr&& other);
    ~asset_ptr();

    // Gets the path to the asset being loaded.
    std::string get_path();

    // If this asset_ptr is valid and points to an asset.
    bool is_valid();

    // If the asset has been loaded.
    bool is_loaded();

    // Gets the current loading state of this asset.
    asset_loading_state get_state();

    // Blocks until the asset has been loaded.
    void wait_for_load();

    // Forces the asset to be reloaded from disk, this is used to support hot reloading.
    //void reload();

    // Gets a hash describing the asset being pointed to. No guarantees are
    // given over stability of the hash or of collisions.
    size_t get_hash() const;

    // Registers a callback for when the state of the asset changes.
    event<>::key_t register_changed_callback(std::function<void()> callback);

    // Unregisters a callback previously registered by register_changed_callback
    void unregister_changed_callback(event<>::key_t key);

    // Operator overloads for all the standard ptr behaviour.
    asset_type& operator*();
    asset_type* operator->();

    asset_ptr& operator=(const asset_ptr& other);
    asset_ptr& operator=(asset_ptr&& other);

    bool operator==(asset_ptr const& other) const;
    bool operator!=(asset_ptr const& other) const;

private:
    asset_manager* m_asset_manager = nullptr;
    asset_state* m_state = nullptr;

};

// ================================================================================================
//  This class implements a basic asset manager. It's responsible for locating, loading
//  and managing the lifetime of any assets loaded from disk.
// 
//  Assets are descibed in the form of yaml files, which always start with a Type and Version
//  property. These determine which asset_loader derived classes is used to load it.
// 
//  Assets are referenced in code using an asset_ptr class. These act as shared_ptr's, assets
//  will remain in memory until all reference are lost.
// 
//  All assets are loaded asyncronously, you can use the asset_ptr interface to query the loading
//  state of an asset. If you attempt to dereference an asset_ptr that has not been loaded yet a
//  stall will occur as the asset is loaded syncronously.
// 
//  This class is thread safe.
// ================================================================================================
class asset_manager
{
public:

    using loader_id = size_t;
    using cache_id = size_t;

    inline static std::string k_compiled_asset_extension = ".compiled";
    inline static std::string k_asset_extension = ".yaml";

    // The platform passed in determines which assets are loaded. Normally this will
    // always by the same as the platform being run on, but can be used to cross-compile
    // assets.
    asset_manager(platform_type asset_platform, config_type asset_config);
    ~asset_manager();

    // Registers a new loader for the given asset type.
    // An id to uniquely identify this loader is returned, this can be later used to unregister it.
    loader_id register_loader(std::unique_ptr<asset_loader>&& loader);

    // Unregisters a previously registered loader.
    // Ensure the loader is not being used.
    void unregister_loader(loader_id id);

    // Registers a new cache for compiled assets.
    // An id to uniquely identify this cache is returned, this can be later used to unregister it.
    cache_id register_cache(std::unique_ptr<asset_cache>&& cache);

    // Unregisters a previously registered cache.
    // Ensure the cache is not being used.
    void unregister_cache(cache_id id);

    // Requests to load an asset described in the yaml file at the given path.
    // 
    // If the asset is not already loaded it will be queued for load, use the asset_ptr
    // interface returned to determine the current loading state.
    // 
    // The higher the priority given the higher the asset will be in the loading queue.
    template<typename T>
    asset_ptr<T> request_asset(const char* path, int32_t priority)
    {
        return asset_ptr<T>(this, create_asset_state(typeid(T), path, priority));
    }

    // Blocks until all pending asset operations have completed.
    void drain_queue();

protected:
    template <typename asset_type>
    friend class asset_ptr;

    // Called for assets in waiting_for_depencies state to check if 
    // they can now proceed to the loading state or not.
    void check_dependency_load_state(asset_state* state);

    // Increments the reference count for a given asset state. May trigger a load.
    void increment_ref(asset_state* state, bool state_lock_held = false);

    // Decrements the reference count for a given asset state. May trigger an unload.
    void decrement_ref(asset_state* state, bool state_lock_held = false);

    // Gets the loader that handles the given type.
    asset_loader* get_loader_for_type(const std::type_info* id);

    // Requests the creation of a state block for the given asset. This state block
    // should be used to construct an asset_ptr.
    asset_state* create_asset_state(const std::type_info& id, const char* path, int32_t priority);

    // Triggers a reload of an asset.
    //void request_reload(asset_state* state);

    // Requests to start loading a given asset state, should only be called by asset_ptr.
    void request_load(asset_state* state);
    void request_load_lockless(asset_state* state);

    // Requests to start unloading a given asset state, should only be called by asset_ptr.
    void request_unload(asset_state* state);
    void request_unload_lockless(asset_state* state);

    // Blocks until the given asset is either loaded or load has failed.
    void wait_for_load(asset_state* state);

    // Runs on the load thread, queues up new loads and unloads.
    void do_work();

    // Begins processing the given asset.
    // Called on coordinator or worker threads.
    void process_asset(asset_state* state);

    // Begins loading the asset.
    // Called on coordinator thread.
    void begin_load(asset_state* state);

    // Begins unloading the asset.
    // Called on coordinator thread.
    void begin_unload(asset_state* state);

    // Performs the actual asset load.
    // Called on worker thread.
    void do_load(asset_state* state);

    // Performs the actual asset unload.
    // Called on worker thread.
    void do_unload(asset_state* state);

    // Sets the current loading state of a given asset.
    // This is used rather than directly setting just to allow
    // for some debug logging.
    void set_load_state(asset_state* state, asset_loading_state new_state);

    // Tries to find the compiled version of an asset by looking through
    // all caches. If not available it will compile the asset and insert
    // into into relevant caches.
    bool get_asset_compiled_path(asset_loader* loader, asset_state* state, std::string& compiled_path);

    // Searches for all asset caches for the given cache key and provides the path to the compiled
    // version if it exists. May migrate assets to closer caches in found in far caches.
    bool search_cache_for_key(const asset_cache_key& cache_key, std::string& compiled_path);

    // Compiles the given asset and stores it in the cache.
    bool compile_asset(asset_cache_key& cache_key, asset_loader* loader, asset_state* state, std::string& compiled_path);

private:
    struct registered_loader
    {
        registered_loader()
            : id(0)
            , loader(nullptr)
        {
        }

        registered_loader(registered_loader&& handle)
            : id(handle.id)
            , loader(std::move(handle.loader))
        {
        }

        registered_loader& operator=(registered_loader&& other)
        {
            id = other.id;
            loader = std::move(other.loader);
            return *this;
        }

        loader_id id;
        std::unique_ptr<asset_loader> loader;
    };

    struct registered_cache
    {
        registered_cache()
            : id(0)
            , cache(nullptr)
        {
        }

        registered_cache(registered_cache&& handle)
            : id(handle.id)
            , cache(std::move(handle.cache))
        {
        }

        registered_cache& operator=(registered_cache&& other)
        {
            id = other.id;
            cache = std::move(other.cache);
            return *this;
        }

        cache_id id;
        std::unique_ptr<asset_cache> cache;
    };

    std::recursive_mutex m_loaders_mutex; // TODO: Change to shared_mutex
    std::vector<registered_loader> m_loaders;
    size_t m_next_loader_id = 0;

    std::mutex m_states_mutex;
    std::condition_variable m_states_convar;
    std::vector<asset_state*> m_states;
    std::vector<asset_state*> m_pending_queue;

    size_t m_max_concurrent_ops = 32;

    std::atomic_size_t m_outstanding_ops;

    std::thread m_load_thread;

    bool m_shutting_down = false;

    platform_type m_asset_platform;
    config_type m_asset_config;

    std::recursive_mutex m_cache_mutex; // TODO: Change to shared_mutex
    std::vector<registered_cache> m_caches;
    size_t m_next_cache_id = 0;

};


template <typename asset_type>
inline asset_ptr<asset_type>::asset_ptr()
    : m_asset_manager(nullptr)
    , m_state(nullptr)
{
}

template <typename asset_type>
inline asset_ptr<asset_type>::asset_ptr(asset_manager* manager, asset_state* state)
    : m_asset_manager(manager)
    , m_state(state)
{
    // Note: No ref increase is required, create_asset_state has already done this before returning.
    //       To avoid anything being nuked between the state being created and the asset_ptr 
    //       incrementing the ref.
}

template <typename asset_type>
inline asset_ptr<asset_type>::asset_ptr(const asset_ptr& other)
    : m_asset_manager(other.m_asset_manager)
    , m_state(other.m_state)
{
    if (m_state)
    {
       m_asset_manager->increment_ref(m_state);
    }
}

template <typename asset_type>
inline asset_ptr<asset_type>::asset_ptr(asset_ptr&& other)
    : m_asset_manager(other.m_asset_manager)
    , m_state(other.m_state)
{
    // Reference doesn't need changing move doesn't effect it.
    other.m_state = nullptr;
}

template <typename asset_type>
inline asset_ptr<asset_type>::~asset_ptr()
{
    if (m_state)
    {
        m_asset_manager->decrement_ref(m_state);
    }
}

template <typename asset_type>
inline std::string asset_ptr<asset_type>::get_path()
{
    if (m_state)
    {
        return m_state->path;
    }
    return "";
}

template <typename asset_type>
inline bool asset_ptr<asset_type>::is_valid()
{
    return m_state != nullptr;
}

template <typename asset_type>
inline bool asset_ptr<asset_type>::is_loaded()
{
    return m_state != nullptr && m_state->loading_state == asset_loading_state::loaded;
}

template <typename asset_type>
inline asset_loading_state asset_ptr<asset_type>::get_state()
{
    return m_state->loading_state;
}

template <typename asset_type>
inline void asset_ptr<asset_type>::wait_for_load()
{
    return m_asset_manager->wait_for_load(m_state);
}

/*
template <typename asset_type>
inline void asset_ptr<asset_type>::reload()
{
    return m_asset_manager->request_reload(m_state);
}
*/


template <typename asset_type>
event<>::key_t asset_ptr<asset_type>::register_changed_callback(std::function<void()> callback)
{
    return m_state->on_change_callback.add(callback);
}

template <typename asset_type>
void asset_ptr<asset_type>::unregister_changed_callback(event<>::key_t key)
{
    m_state->on_change_callback.remove(key);
}

template <typename asset_type>
inline asset_type& asset_ptr<asset_type>::operator*()
{
    if (m_state->loading_state == asset_loading_state::failed)
    {
        if (m_state->default_asset != nullptr)
        {
            return *static_cast<asset_type*>(m_state->default_asset);
        }
        else
        {
            db_fatal(engine, "Attempted to dereference asset that failed to load '%s', and no default asset available.", get_path().c_str());
        }
    }
    else if (m_state->loading_state != asset_loading_state::loaded)
    {
        wait_for_load();
    }
    return *static_cast<asset_type*>(m_state->instance);
}

template <typename asset_type>
inline asset_type* asset_ptr<asset_type>::operator->()
{
    return &(operator*());
}

template <typename asset_type>
inline asset_ptr<asset_type>& asset_ptr<asset_type>::operator=(const asset_ptr<asset_type>& other)
{
    m_asset_manager = other.m_asset_manager;
    m_state = other.m_state;

    if (m_state)
    {
        m_asset_manager->increment_ref(m_state);
    }

    return *this;
}

template <typename asset_type>
inline asset_ptr<asset_type>& asset_ptr<asset_type>::operator=(asset_ptr<asset_type>&& other)
{
    m_asset_manager = other.m_asset_manager;
    m_state = other.m_state;

    other.m_state = nullptr;

    // Reference doesn't need changing move doesn't effect it.

    return *this;
}

template <typename asset_type>
inline size_t asset_ptr<asset_type>::get_hash() const
{
    size_t hash = 0;
    ws::hash_combine(hash, *reinterpret_cast<const size_t*>(&m_state));
    ws::hash_combine(hash, *reinterpret_cast<const size_t*>(&m_asset_manager));
    return hash;
}

template <typename asset_type>
inline bool asset_ptr<asset_type>::operator==(asset_ptr<asset_type> const& other) const
{
    return m_state == other.m_state &&
           m_asset_manager == other.m_asset_manager;
}

template <typename asset_type>
inline bool asset_ptr<asset_type>::operator!=(asset_ptr<asset_type> const& other) const
{
    return !operator==(other);
}

}; // namespace workshop
