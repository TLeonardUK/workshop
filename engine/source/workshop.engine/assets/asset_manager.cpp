// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/assets/asset_manager.h"
#include "workshop.engine/assets/asset_loader.h"
#include "workshop.core/debug/debug.h"
#include "workshop.core/async/async.h"

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

static std::array<const char*, static_cast<int>(asset_loading_state::COUNT)> k_loading_state_strings = {
    "Unloaded",
    "Unloading",
    "Loading",
    "Loaded",
    "Failed"
};

};

asset_manager::asset_manager()
{
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

void asset_manager::drain_queue()
{
    std::unique_lock lock(m_states_mutex);

    while (m_pending_queue.size() > 0)
    {
        m_states_convar.wait(lock);
    }
}

asset_state* asset_manager::create_asset_state(const std::type_info& id, const char* path, int32_t priority)
{
    std::unique_lock lock(m_states_mutex);

    asset_state* state = new asset_state();
    state->default_asset = nullptr;
    state->loading_state = asset_loading_state::unloaded;
    state->priority = priority;
    state->references = 0;
    state->type_id = &id;
    state->path = path;

    asset_loader* loader = get_loader_for_type(state->type_id);
    if (loader != nullptr)
    {
        state->default_asset = loader->get_default_asset();
    }

    m_states.push_back(state);

    return state;
}

void asset_manager::request_load(asset_state* state)
{
    std::unique_lock lock(m_states_mutex);
    
    state->should_be_loaded = true;

    if (!state->is_pending)
    {
        state->is_pending = true;
        m_pending_queue.push(state);

        m_states_convar.notify_all();
    }
}

void asset_manager::request_unload(asset_state* state)
{
    std::unique_lock lock(m_states_mutex);

    state->should_be_loaded = false;

    if (!state->is_pending)
    {
        state->is_pending = true;
        m_pending_queue.push(state);

        m_states_convar.notify_all();
    }
}

void asset_manager::wait_for_load(asset_state* state)
{
    std::unique_lock lock(m_states_mutex);

    while (true)
    {
        if (state->loading_state == asset_loading_state::loaded ||
            state->loading_state == asset_loading_state::failed ||
            !state->is_pending)
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
        if (m_outstanding_ops.load() < k_max_concurrent_ops)
        {
            if (m_pending_queue.size() > 0)
            {
                asset_state* state = m_pending_queue.front();
                m_pending_queue.pop();

                process_asset(state);

                continue;
            }
        }

        m_states_convar.wait(lock);
    }
}

void asset_manager::process_asset(asset_state* state)
{
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

    async("Load Asset", task_queue::loading, [this, state]() {
    
        do_load(state);

        {
            std::unique_lock lock(m_states_mutex);
            set_load_state(state, state->instance ? asset_loading_state::loaded : asset_loading_state::failed);

            // Process the asset again incase the requested state
            // has changed during this process.
            process_asset(state);

            m_states_convar.notify_all();
        }
    
    });
}

void asset_manager::begin_unload(asset_state* state)
{
    db_assert(state->loading_state == asset_loading_state::loaded);
    set_load_state(state, asset_loading_state::unloading);

    async("Unload Asset", task_queue::loading, [this, state]() {
    
        do_unload(state);

        {
            std::unique_lock lock(m_states_mutex);
            set_load_state(state, asset_loading_state::unloaded);

            // Process the asset again incase the requested state
            // has changed during this process.
            process_asset(state);

            m_states_convar.notify_all();
        }
    
    });
}

void asset_manager::do_load(asset_state* state)
{
    asset_loader* loader = get_loader_for_type(state->type_id);
    if (loader != nullptr)
    {
        state->instance = loader->load(state->path.c_str());
    }
}

void asset_manager::do_unload(asset_state* state)
{
    asset_loader* loader = get_loader_for_type(state->type_id);
    if (loader != nullptr)
    {
        loader->unload(state->instance);
    }
}

void asset_manager::set_load_state(asset_state* state, asset_loading_state new_state)
{
    db_log(asset, "[%s] %s", state->path.c_str(), k_loading_state_strings[static_cast<int>(new_state)]);
    state->loading_state = new_state;
}

}; // namespace workshop
