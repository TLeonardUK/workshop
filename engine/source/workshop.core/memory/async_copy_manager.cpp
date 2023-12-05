// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/memory/async_copy_manager.h"
#include "workshop.core/debug/debug.h"
#include "workshop.core/perf/profile.h"

#include <Windows.h>

namespace ws {

async_copy_request::async_copy_request(async_copy_manager* manager, size_t id)
    : m_manager(manager)
    , m_id(id)
{
}

async_copy_request::~async_copy_request()
{
}

bool async_copy_request::is_complete()
{
    std::unique_lock lock(m_manager->m_mutex);
    return !m_manager->m_states.contains(m_id);

}

void async_copy_request::wait()
{
    while (m_manager->m_running)
    {
        std::unique_lock lock(m_manager->m_mutex);
        if (is_complete())
        {
            break;
        }

        m_manager->m_cond_var.wait(lock);
    }
}

async_copy_manager::async_copy_manager()
{
    m_running = true;
    m_thread = std::make_unique<std::thread>([this]() {
        db_set_thread_name("async copy thread");
        worker_thread();
    });
}

async_copy_manager::~async_copy_manager()
{
    m_running = false;
    m_cond_var.notify_all();
    m_thread->join();
}

void async_copy_manager::worker_thread()
{
    while (m_running)
    {
        // Grab new work, if none is available then sleep.
        state work;
        bool found_work = false;
        
        while (m_running)
        {
            std::unique_lock lock(m_mutex);
            if (m_states.empty())
            {
                m_cond_var.wait(lock);
            }
            else
            {
                work = (*m_states.begin()).second;
                found_work = true;
                break;
            }
        }

        // Perform the actual work.
        if (found_work)
        {
            profile_marker(profile_colors::task, "async memcpy");

            memcpy(work.destination, work.source, work.size);

            // Erase from work queue
            {
                std::unique_lock lock(m_mutex);
                m_states.erase(work.id);
            }
        }
    }
}

async_copy_request async_copy_manager::request_memcpy(void* destination, void* source, size_t size)
{
    std::unique_lock lock(m_mutex);

    state new_state;
    new_state.id = m_id_counter.fetch_add(1);
    new_state.destination = destination;
    new_state.source = source;
    new_state.size = size;
    m_states[new_state.id] = new_state;

    m_cond_var.notify_all();

    return async_copy_request(this, new_state.id);
}

}; // namespace workshop
