// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/singleton.h"
#include "workshop.core/hashing/string_hash.h"

#include <array>
#include <unordered_map>

namespace ws {

class async_copy_manager;

// Represents a copy that was requested through one of the calls to async_copy_manager's
// public interface. Can be kept indefinitely and used to query the current state of the
// copy.
class async_copy_request
{
public:
    async_copy_request(async_copy_manager* manager, size_t id);
    ~async_copy_request();

    // Returns true if this request has finished.
    bool is_complete();

    // Waits until the copy has completed. Avoid using as it will cause stalls, monitor
    // is_complete instead.
    void wait();

private:
    async_copy_manager* m_manager;
    size_t m_id;

};

// This class is responsible for asynchronously performing copy operations, such as memcpy's. 
// It can be used to avoid stalls by doing large copies (such as bulk texture data) on 
// important threads.
//
// For some platforms we could use this to wrap DMA transfers.
class async_copy_manager
	: public singleton<async_copy_manager>
{
public:

    async_copy_manager();
    ~async_copy_manager();

    // Requests copying size bytes of data from source into destination. 
    // Both destination and source must remain valid until copy is complete.
    async_copy_request request_memcpy(void* destination, void* source, size_t size);

private:
    friend class async_copy_request;

    void worker_thread();

private:

    struct state
    {
        size_t id;
        void* destination;
        void* source;
        size_t size;
    };

    std::unique_ptr<std::thread> m_thread;

    bool m_running = false;

    std::mutex m_mutex;
    std::condition_variable m_cond_var;

    std::atomic_size_t m_id_counter;

    std::unordered_map<size_t, state> m_states;

};

}; // namespace ws
