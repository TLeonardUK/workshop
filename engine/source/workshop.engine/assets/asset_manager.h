// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/debug/debug.h"
#include "workshop.engine/assets/asset_loader.h"

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

    // Gets the current loading state of this asset.
    asset_loading_state get_state();

    // Blocks until the asset has been loaded.
    void wait_for_load();

    // Operator overloads for all the standard ptr behaviour.
    asset_type& operator*();
    asset_type* operator->();

    asset_ptr& operator=(const asset_ptr& other);
    asset_ptr& operator=(asset_ptr&& other);

    bool operator==(asset_ptr const& other) const;
    bool operator!=(asset_ptr const& other) const;

protected:
    void increment_ref();
    void decrement_ref();

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

    asset_manager();
    ~asset_manager();

    // Registers a new loader for the given asset type.
    // An id to uniquely identify this loader is returned, this can be later used to unregister it.
    loader_id register_loader(std::unique_ptr<asset_loader>&& loader);

    // Unregisters a previously registered loader.
    // Ensure the loader is not being used.
    void unregister_loader(loader_id id);

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

    // Gets the loader that handles the given type.
    asset_loader* get_loader_for_type(const std::type_info* id);

    // Requests the creation of a state block for the given asset. This state block
    // should be used to construct an asset_ptr.
    asset_state* create_asset_state(const std::type_info& id, const char* path, int32_t priority);

    // Requests to start loading a given asset state, should only be called by asset_ptr.
    void request_load(asset_state* state);

    // Requests to start unloading a given asset state, should only be called by asset_ptr.
    void request_unload(asset_state* state);

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

    std::recursive_mutex m_loaders_mutex;
    std::vector<registered_loader> m_loaders;

    std::mutex m_states_mutex;
    std::condition_variable m_states_convar;
    std::vector<asset_state*> m_states;
    std::queue<asset_state*> m_pending_queue;

    constexpr inline static const size_t k_max_concurrent_ops = 5;

    std::atomic_size_t m_outstanding_ops;

    std::thread m_load_thread;

    size_t m_next_loader_id = 0;

    bool m_shutting_down = false;

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
    if (m_state)
    {
        increment_ref();
    }
}

template <typename asset_type>
inline asset_ptr<asset_type>::asset_ptr(const asset_ptr& other)
    : m_asset_manager(other.m_manager)
    , m_state(other.m_state)
{
    if (m_state)
    {
        increment_ref();
    }
}

template <typename asset_type>
inline asset_ptr<asset_type>::asset_ptr(asset_ptr&& other)
    : m_asset_manager(other.m_asset_manager)
    , m_state(other.m_state)
{
    // Reference doesn't need changing move doesn't effect it.
}

template <typename asset_type>
inline asset_ptr<asset_type>::~asset_ptr()
{
    if (m_state)
    {
        decrement_ref();       
    }
}

template <typename asset_type>
inline void asset_ptr<asset_type>::increment_ref()
{
    size_t result = m_state->references.fetch_add(1);
    if (result == 0)
    {
        m_asset_manager->request_load(m_state);
    }
}

template <typename asset_type>
inline void asset_ptr<asset_type>::decrement_ref()
{
    size_t result = m_state->references.fetch_sub(1);
    if (result == 1)
    {
        m_asset_manager->request_unload(m_state);
    }
}

template <typename asset_type>
inline std::string asset_ptr<asset_type>::get_path()
{
    if (m_state)
    {
        return m_state->m_path;
    }
    return "";
}

template <typename asset_type>
inline bool asset_ptr<asset_type>::is_valid()
{
    return m_state != nullptr;
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
    return *static_cast<asset_type*>(m_state->asset);
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
        increment_ref();
    }
}

template <typename asset_type>
inline asset_ptr<asset_type>& asset_ptr<asset_type>::operator=(asset_ptr<asset_type>&& other)
{
    m_asset_manager = other.m_asset_manager;
    m_state = other.m_state;

    // Reference doesn't need changing move doesn't effect it.
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
