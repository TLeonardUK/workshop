// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/debug/debug.h"
#include "workshop.core/perf/timer.h"
#include "workshop.core/utils/event.h"
#include "workshop.core/utils/singleton.h"
#include "workshop.assets/asset_loader.h"
#include "workshop.assets/asset_importer.h"
#include "workshop.assets/asset_cache.h"

#include <thread>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <typeindex>

namespace ws {

class asset;
class asset_manager;

// Loading state of an asset.
enum class asset_loading_state
{
    unloaded,
    unloading,
    loading,
    compiling,
    waiting_for_dependencies,
    loaded,
    failed,

    COUNT
};

static const char* asset_loading_state_strings[static_cast<int>(asset_loading_state::COUNT)] = {
    "unloaded",
    "unloading",
    "loading",
    "compiling",
    "waiting for dependencies",
    "loaded",
    "failed",
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

    asset_cache_key cache_key;

    const std::type_info* type_id;

    std::vector<asset_state*> dependencies;
    std::vector<asset_state*> depended_on_by;

    std::vector<std::unique_ptr<virtual_file_system_watcher>> file_watchers;

    bool is_for_hot_reload = false;
    asset_state* hot_reload_state = nullptr;

    // Version number thats bumped each time the asset changes.
    std::atomic_size_t version = 0;
    
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
class asset_ptr_base
{
public:
    asset_ptr_base();
    asset_ptr_base(asset_manager* manager, asset_state* state, const std::type_info* type);

    // Gets the path to the asset being loaded.
    std::string get_path();

    // If this asset_ptr is valid and points to an asset.
    bool is_valid();

    // If the asset has been loaded.
    bool is_loaded();

    // Gets the version of the asset. Each time the asset changes the version is bumped.
    size_t get_version();

    // Gets the current loading state of this asset.
    asset_loading_state get_state();

    // Blocks until the asset has been loaded.
    void wait_for_load();

    // Forces the asset to be reloaded from disk, this is used to support hot reloading.
    //void reload();

    // Gets a hash describing the asset being pointed to. No guarantees are
    // given over stability of the hash or of collisions.
    size_t get_hash() const;

    // Gets the manager that handles this assets loading.
    asset_manager* get_asset_manager();

    // Registers a callback for when the state of the asset changes.
    event<>::key_t register_changed_callback(std::function<void()> callback);

    // Unregisters a callback previously registered by register_changed_callback
    void unregister_changed_callback(event<>::key_t key);

    // Resets the point so it no longer points to anything.
    void reset();

    // Switches the asset being pointed to and invalidates the current asset held.
    void set_path(const char* path);

protected:

    // Swaps the state this pointer holds for a different one.
    void swap_state(asset_state* state);

protected:

    asset_manager* m_asset_manager = nullptr;
    asset_state* m_state = nullptr;
    const std::type_info* m_type;

};

template <typename asset_type>
class asset_ptr : public asset_ptr_base
{
public:
    // This is used for reflection when using REFLECT_FIELD_REF
    using super_type_t = asset_ptr_base;

    asset_ptr();
    asset_ptr(asset_manager* manager, asset_state* state);
    asset_ptr(const asset_ptr& other);
    asset_ptr(asset_ptr&& other);
    ~asset_ptr();

    // Gets the asset or asserts if not loaded.
    asset_type* get();

    // Operator overloads for all the standard ptr behaviour.
    asset_type& operator*();
    asset_type* operator->();

    asset_ptr& operator=(const asset_ptr& other);
    asset_ptr& operator=(asset_ptr&& other);

    bool operator==(asset_ptr const& other) const;
    bool operator!=(asset_ptr const& other) const;

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
    : public singleton<asset_manager>
{
public:

    using loader_id = size_t;
    using importer_id = size_t;
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
        return asset_ptr<T>(this, create_asset_state(typeid(T), path, priority, false));
    }

    // Blocks until all pending asset operations have completed.
    void drain_queue();

    // Returns true if any hot reloads are pending and apply_hot_reloads is needed.
    bool has_pending_hot_reloads();

    // Performs any needed hot reload swapping. Done explicitly so higher level code
    // can control any syncronisation problems.
    void apply_hot_reloads();

    // Runs callback for every asset state the manager is currently handling. 
    // This is mostly here for debugging, its not fast and will block loading, so don't use it anywhere time critical.
    using visit_callback_t = std::function<void(asset_state* state)>;
    void visit_assets(visit_callback_t callback);

    // Registers a new importer for the given asset type.
    // An id to uniquely identify this importer is returned, this can be later used to unregister it.
    importer_id register_importer(std::unique_ptr<asset_importer>&& loader);

    // Unregisters a previously registered importer.
    // Ensure the importer is not being used.
    void unregister_importer(importer_id id);

    // Gets a list of all asset importers. Be careful as the pointers can be unregistered
    // and become stale, so do not hold them at any point unregister_importer could be called.
    std::vector<asset_importer*> get_asset_importers();

    // Finds and returns the first registered importer that supports the given extension.
    asset_importer* get_importer_for_extension(const char* extension);

    // Gets the loader from the descriptor type stored in the corresponding asset files.
    asset_loader* get_loader_for_descriptor_type(const char* type);

    // Gets the platform this asset manager is loading/compiling assets for.
    platform_type get_asset_platform();

    // Gets the config this asset manager is loading/compiling assets for.
    config_type get_asset_config();

    // Gets the loader that uses the handles the given asset type.
    asset_loader* get_loader_for_type(std::type_index type);

    template <typename asset_type>
    asset_loader* get_loader_for_type()
    {
        return get_loader_for_type(typeid(asset_type));
    }

protected:
    friend class asset_ptr_base;
    
    template <typename asset_type>
    friend class asset_ptr;

    template <typename value_type>
    friend void stream_serialize(stream& out, asset_ptr_base& v);

    template <typename value_type>
    friend void yaml_serialize(YAML::Node& out, bool is_loading, asset_ptr_base& value);

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
    asset_state* create_asset_state(const std::type_info& id, const char* path, int32_t priority, bool is_hot_reload);
    asset_state* create_asset_state_lockless(const std::type_info& id, const char* path, int32_t priority, bool is_hot_reload);

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

    // Registers file watchers to monitor for hot reloading of asset.
    void start_watching_for_reload(asset_state* state);
    void stop_watching_for_reload(asset_state* state);

    // Queus a state for hot reload.
    void hot_reload(asset_state* state);

    // Cleans up a state that has been unloaded and no longer required.
    void delete_state(asset_state* state);

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
    
    struct registered_importer
    {
        registered_importer()
            : id(0)
            , importer(nullptr)
        {
        }

        registered_importer(registered_importer&& handle)
            : id(handle.id)
            , importer(std::move(handle.importer))
        {
        }

        registered_importer& operator=(registered_importer&& other)
        {
            id = other.id;
            importer = std::move(other.importer);
            return *this;
        }

        importer_id id;
        std::unique_ptr<asset_importer> importer;
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

    std::recursive_mutex m_importers_mutex; // TODO: Change to shared_mutex
    std::vector<registered_importer> m_importers;
    size_t m_next_importer_id = 0;

    std::mutex m_states_mutex;
    std::condition_variable m_states_convar;
    std::vector<asset_state*> m_states;
    std::vector<asset_state*> m_pending_queue;
    std::vector<asset_state*> m_pending_hot_reload;

    std::vector<asset_state*> m_hot_reload_queue;

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

inline std::string asset_ptr_base::get_path()
{
    if (m_state)
    {
        return m_state->path;
    }
    return "";
}

inline bool asset_ptr_base::is_valid()
{
    return m_state != nullptr;
}

inline bool asset_ptr_base::is_loaded()
{
    return m_state != nullptr && m_state->loading_state == asset_loading_state::loaded;
}

inline size_t asset_ptr_base::get_version()
{
    return m_state != nullptr ? m_state->version.load() : 0llu;
}

inline asset_loading_state asset_ptr_base::get_state()
{
    return m_state->loading_state;
}

inline void asset_ptr_base::wait_for_load()
{
    return m_asset_manager->wait_for_load(m_state);
}

inline event<>::key_t asset_ptr_base::register_changed_callback(std::function<void()> callback)
{
    return m_state->on_change_callback.add(callback);
}

inline void asset_ptr_base::unregister_changed_callback(event<>::key_t key)
{
    m_state->on_change_callback.remove(key);
}

inline size_t asset_ptr_base::get_hash() const
{
    size_t hash = 0;
    ws::hash_combine(hash, *reinterpret_cast<const size_t*>(&m_state));
    ws::hash_combine(hash, *reinterpret_cast<const size_t*>(&m_asset_manager));
    return hash;
}

inline asset_manager* asset_ptr_base::get_asset_manager()
{
    return m_asset_manager;
}

inline void asset_ptr_base::reset()
{
    if (m_state)
    {
        m_asset_manager->decrement_ref(m_state);
        m_state = nullptr;
    }
}

inline void asset_ptr_base::swap_state(asset_state* state)
{
    if (m_state)
    {
        m_asset_manager->decrement_ref(m_state);
        m_state = nullptr;
    }
    
    // NOTE: This is used for states returned by create_asset_state, which should have already
    //       done in the reference increment, so avoid doing it again here.
    m_state = state;
}

inline void asset_ptr_base::set_path(const char* path)
{
    if (path[0] == '\0')
    {
        reset();
    }
    else
    {
        if (m_asset_manager == nullptr)
        {
            m_asset_manager = &asset_manager::get();
        }

        asset_state* new_state = m_asset_manager->create_asset_state(*m_type, path, 0, false);
        swap_state(new_state);
    }
}

inline asset_ptr_base::asset_ptr_base()
    : m_asset_manager(nullptr)
    , m_state(nullptr)
    , m_type(&typeid(void))
{
}

inline asset_ptr_base::asset_ptr_base(asset_manager* manager, asset_state* state, const std::type_info* type)
    : m_asset_manager(manager)
    , m_state(state)
    , m_type(type)
{
}


template <typename asset_type>
inline asset_ptr<asset_type>::asset_ptr()
    : asset_ptr_base(nullptr, nullptr, &typeid(asset_type))
{
}

template <typename asset_type>
inline asset_ptr<asset_type>::asset_ptr(asset_manager* manager, asset_state* state)
    : asset_ptr_base(manager, state, &typeid(asset_type))
{
    // Note: No ref increase is required, create_asset_state has already done this before returning.
    //       To avoid anything being nuked between the state being created and the asset_ptr 
    //       incrementing the ref.
}

template <typename asset_type>
inline asset_ptr<asset_type>::asset_ptr(const asset_ptr& other)
{
    m_asset_manager = other.m_asset_manager;
    m_state = other.m_state;
    m_type = other.m_type;

    if (m_state)
    {
       m_asset_manager->increment_ref(m_state);
    }
}

template <typename asset_type>
inline asset_ptr<asset_type>::asset_ptr(asset_ptr&& other)
{
    m_asset_manager = other.m_asset_manager;
    m_state = other.m_state;
    m_type = other.m_type;

    // Reference doesn't need changing move doesn't effect it.
    other.m_state = nullptr;
}

template <typename asset_type>
inline asset_ptr<asset_type>::~asset_ptr()
{
    if (m_state)
    {
        m_asset_manager->decrement_ref(m_state);
        m_state = nullptr;
    }
}

template <typename asset_type>
asset_type* asset_ptr<asset_type>::get()
{
    if (m_state->loading_state == asset_loading_state::failed)
    {
        if (m_state->default_asset != nullptr)
        {
            return static_cast<asset_type*>(m_state->default_asset);
        }
        else
        {
            db_fatal(engine, "Attempted to dereference asset that failed to load '%s', and no default asset available.", get_path().c_str());
        }
    }
    else if (m_state->loading_state != asset_loading_state::loaded)
    {
        db_warning(engine, "Attempted to dereference asset that is not loaded '%s'. Forcing a blocking load.", get_path().c_str());
        wait_for_load();
    }
    return static_cast<asset_type*>(m_state->instance);
}

template <typename asset_type>
inline asset_type& asset_ptr<asset_type>::operator*()
{
    return *get();
}

template <typename asset_type>
inline asset_type* asset_ptr<asset_type>::operator->()
{
    return get();
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

template<>
inline void stream_serialize(stream& out, asset_ptr_base& v)
{
    std::string path = v.get_path();
    stream_serialize(out, path);

    if (!out.can_write())
    {
        v.set_path(path.c_str());
    }
}

template<>
inline void yaml_serialize(YAML::Node& out, bool is_loading, asset_ptr_base& v)
{
    std::string path = v.get_path();

    yaml_serialize(out, is_loading, path);

    if (is_loading)
    {
        v.set_path(path.c_str());
    }
}

}; // namespace workshop
