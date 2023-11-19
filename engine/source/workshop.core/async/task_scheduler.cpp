// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/async/task_scheduler.h"
#include "workshop.core/debug/debug.h"
#include "workshop.core/containers/string.h"
#include "workshop.core/perf/profile.h"

#include <algorithm>
#include <thread>
#include <future>
#include <array>
#include <unordered_set>

namespace ws {

task_handle::task_handle()
    : m_task_scheduler(nullptr)
    , m_index(task_scheduler::k_invalid_task_index)
{
}

task_handle::task_handle(task_scheduler* scheduler, task_index_t index)
    : m_task_scheduler(scheduler)
    , m_index(index)
{
    db_assert(is_valid());

    increment_ref();
}

task_handle::task_handle(task_handle&& other)
{
    m_task_scheduler = std::move(other.m_task_scheduler);
    other.m_task_scheduler = nullptr;
    m_index = other.m_index;
}

task_handle::task_handle(const task_handle& other)
    : m_task_scheduler(other.m_task_scheduler)
    , m_index(other.m_index)
{
    if (is_valid())
    {
        increment_ref();
    }
}

task_handle::~task_handle()
{
    if (is_valid())
    {
        decrement_ref();
    }
}

bool task_handle::is_valid()
{
    if (m_task_scheduler == nullptr)
    {
        return false;
    }

    if (m_index == task_scheduler::k_invalid_task_index)
    {
        return false;
    }

    return true;
}

bool task_handle::is_complete()
{
    db_assert(is_valid() && is_dispatched());

    return m_task_scheduler->get_task_run_state(m_index) == task_scheduler::task_run_state::complete;
}

bool task_handle::is_dispatched()
{
    db_assert(is_valid());

    return m_task_scheduler->get_task_run_state(m_index) != task_scheduler::task_run_state::pending_dispatch;
}

void task_handle::add_dependency(const task_handle& other)
{
    db_assert(is_valid() && !is_dispatched());

    m_task_scheduler->add_task_dependency(m_index, other.m_index);
}

void task_handle::dispatch()
{
    db_assert(is_valid() && !is_dispatched());

    m_task_scheduler->dispatch_task(m_index);
}

void task_handle::wait(bool can_help)
{
    db_assert(is_valid());

    m_task_scheduler->wait_for_task(m_index, can_help);
}

void task_handle::reset()
{
    if (is_valid())
    {
        decrement_ref();
    }

    m_task_scheduler = nullptr;
    m_index = 0;
}

task_handle& task_handle::operator=(const task_handle& other)
{
    if (is_valid())
    {
        decrement_ref();
    }

    m_task_scheduler = other.m_task_scheduler;
    m_index = other.m_index;
    
    if (is_valid())
    {
        increment_ref();
    }

    return *this;
}

task_handle& task_handle::operator=(task_handle&& other)
{
    if (is_valid())
    {
        decrement_ref();
    }

    m_task_scheduler = std::move(other.m_task_scheduler);
    other.m_task_scheduler = nullptr;

    m_index = other.m_index;

    return *this;
}

bool task_handle::operator==(task_handle const& other) const
{
    return m_task_scheduler == other.m_task_scheduler &&
           m_index == other.m_index;
}

bool task_handle::operator!=(task_handle const& other) const
{
    return !operator==(other);
}

void task_handle::increment_ref()
{
    task_scheduler::task_state& state = m_task_scheduler->get_task_state(m_index);
    state.references.fetch_add(1);
}

void task_handle::decrement_ref()
{
    task_scheduler::task_state& state = m_task_scheduler->get_task_state(m_index);
    if (state.references.fetch_sub(1) == 1)
    {
       m_task_scheduler->try_release_task(m_index);
    }
}

task_index_t task_handle::get_task_index() const
{
    return m_index;
}


task_scheduler::task_scheduler(init_state& states)
{
    db_assert(states.worker_count > 0);

    m_workers.resize(states.worker_count);

    // Mark all task indices as free.
    for (size_t i = 0; i < k_max_tasks; i++)
    {
        m_free_task_indices.push((uint32_t)i);
    }

    // Assign queues to workers.
    size_t nextWorkerIndex = 0;
    for (size_t i = 0; i < static_cast<int>(task_queue::COUNT); i++)
    {
        float weight = states.queue_weights[i];
        db_assert(weight >= 0.0f && weight <= 1.0f);

        size_t worker_count = std::max(1, static_cast<int>(ceilf(states.worker_count * weight)));
        m_queues[i].worker_count = worker_count;

        for (size_t j = 0; j < worker_count; j++)
        {
            m_workers[nextWorkerIndex].queues.insert(&m_queues[i]);
            nextWorkerIndex = (nextWorkerIndex + 1) % states.worker_count;
        }
    }

    // Start threads for each worker.
    for (size_t i = 0; i < states.worker_count; i++)
    {
        worker_state& worker = m_workers[i];
        worker.thread = std::make_unique<std::thread>([=, &worker]() {
            
            db_set_thread_name(string_format("Task Worker %zi", i));
            worker_entry(worker);

        });
    }

    db_log(core, "Task scheduler memory usage: %.2f mb", sizeof(*this) / (1024.0f * 1024.0f));
}

task_scheduler::~task_scheduler()
{
    m_shutting_down = true;
    for (size_t i = 0; i < m_workers.size(); i++)
    {
        worker_state& worker = m_workers[i];
        worker.work_sempahore.release();
        worker.thread->join();        
        worker.thread.reset();
    }
}

size_t task_scheduler::get_worker_count(task_queue queue)
{
    return m_queues[static_cast<int>(queue)].worker_count;
}

task_handle task_scheduler::create_task(const char* name, task_queue queue, task_scheduler::task_function workload)
{
    task_index_t index = alloc_task_index();
    task_state& state = m_tasks[index];

    strcpy_s(state.name, sizeof(state.name), name);
    state.index = index;
    state.queue = queue;
    state.state = task_run_state::pending_dispatch;
    state.work = workload;
    state.dependents.clear();

    return task_handle(this, index);
}

std::vector<task_handle> task_scheduler::create_tasks(size_t count, const char* name, task_queue queue, task_function workload)
{
    std::unique_lock lock(m_task_allocation_mutex);

    std::vector<task_handle> result;
    result.resize(count);

    for (size_t i = 0; i < count; i++)
    {
        task_index_t index = alloc_task_index_lockless();
        task_state& state = m_tasks[index];

        strcpy_s(state.name, sizeof(state.name), name);
        state.index = index;
        state.queue = queue;
        state.state = task_run_state::pending_dispatch;
        state.work = workload;
        state.dependents.clear();

        result[i] = task_handle(this, index);
    }

    return result;
}

void task_scheduler::drain()
{
    std::unique_lock lock(m_task_mutex);

    while (true)
    {
        bool is_drained = true;

        for (size_t i = 0; i < m_tasks.size(); i++)
        {
            if (m_tasks[i].state == task_run_state::pending_dispatch ||
                m_tasks[i].state == task_run_state::pending_run)
            {
                is_drained = false;
            }
        }

        if (is_drained)
        {
            break;
        }

        m_task_changed_convar.wait(lock);
    }
}

task_index_t task_scheduler::alloc_task_index()
{
    std::unique_lock lock(m_task_allocation_mutex);
    return alloc_task_index_lockless();
}

task_index_t task_scheduler::alloc_task_index_lockless()
{
    db_assert(!m_free_task_indices.empty());

    static size_t peak_in_use = 0;
    size_t in_use = k_max_tasks - m_free_task_indices.size();
    if (in_use > peak_in_use)
    {
        peak_in_use = in_use;
        db_log(renderer, "in-use:%zi", in_use);
    }

    task_index_t index = m_free_task_indices.front();
    m_free_task_indices.pop();

    return index;
}

void task_scheduler::free_task_index(task_index_t task_index)
{
    std::unique_lock lock(m_task_allocation_mutex);
    m_tasks[task_index].state = task_run_state::unallocated;
    m_free_task_indices.push(task_index);
}

void task_scheduler::try_release_task(task_index_t index)
{
    std::unique_lock lock(m_task_mutex);
    try_release_task_lockless(index);
}

void task_scheduler::try_release_task_lockless(task_index_t index)
{
    if (m_tasks[index].state == task_run_state::complete &&
        m_tasks[index].references.load() == 0)
    {
        free_task_index(index);
    }
}

task_scheduler::task_run_state task_scheduler::get_task_run_state(task_index_t task_index)
{
    return m_tasks[task_index].state;
}

void task_scheduler::add_task_dependency(task_index_t task_index, task_index_t dependency_index)
{
    task_state& state = m_tasks[task_index];
    task_state& dependencyState = m_tasks[dependency_index];
    db_assert(state.state == task_run_state::pending_dispatch);
    db_assert(dependencyState.state == task_run_state::pending_dispatch);

    dependencyState.dependents.push_back(task_index);
    state.outstanding_dependencies++;
}

void task_scheduler::dispatch_task(task_index_t index)
{
    std::scoped_lock lock(m_task_dispatch_mutex);

    std::vector<task_index_t> indices;
    indices.push_back(index);

    dispatch_tasks_lockless(indices);
}

void task_scheduler::dispatch_tasks_lockless(const std::vector<task_index_t>& indices)
{
    std::array<bool, static_cast<int>(task_queue::COUNT)> queues_used = {};

    for (task_index_t index : indices)
    {
        task_state& state = m_tasks[index];
        db_assert(state.state == task_run_state::pending_dispatch ||
                  state.state == task_run_state::pending_dependencies);

        // If we are pending dependencies we will be dispatched when they are complete.
        if (state.outstanding_dependencies > 0)
        {
            state.state = task_run_state::pending_dependencies;
            continue;
        }

        queue_state& queue_state = m_queues[static_cast<int>(state.queue)];
    
        {
            std::unique_lock lock(queue_state.work_mutex);
            queue_state.work.push(index);
            state.state = task_run_state::pending_run;
        }

        queues_used[static_cast<int>(state.queue)] = true;
    }

    // Wake up all workers that can process the queue the task was pushed into.
    for (size_t i = 0; i < m_workers.size(); i++)
    {
        worker_state& worker = m_workers[i];

        for (size_t j = 0; j < queues_used.size(); j++)
        {
            if (!queues_used[j])
            {
                continue;
            }

            queue_state& queue_state = m_queues[j];
            if (worker.queues.contains(&queue_state))
            {
                // TODO: This bad boy is slow as fuck when we have a couple of dozen workers, we need to rejig this.
                worker.work_sempahore.release();
            }
        }
    }

    // Notify any helpers that a task has been queued.
    {
        std::unique_lock lock(m_task_mutex);    
        m_task_changed_convar.notify_all();
    }
}

void task_scheduler::dispatch_tasks(const std::vector<task_handle>& handles)
{
    std::scoped_lock lock(m_task_dispatch_mutex);

    std::vector< task_index_t> indices;
    for (const task_handle& handle : handles)
    {
        indices.push_back(handle.get_task_index());
    }

    dispatch_tasks_lockless(indices);
}

void task_scheduler::wait_for_task(task_index_t index, bool can_help)
{
    std::vector<task_index_t> indicies;
    indicies.push_back(index);

    if (can_help)
    {
        wait_for_tasks_helping(indicies);
    }
    else
    {
        wait_for_tasks_no_help(indicies);
    }
}

void task_scheduler::wait_for_tasks(const std::vector<task_handle>& handles, bool can_help)
{
    std::vector<task_index_t> indicies;
    indicies.resize(handles.size());

    for (size_t i = 0; i < handles.size(); i++)
    {
        indicies[i] = handles[i].get_task_index();
    }

    if (can_help)
    {
        wait_for_tasks_helping(indicies);
    }
    else
    {
        wait_for_tasks_no_help(indicies);
    }
}

bool task_scheduler::are_tasks_complete(const std::vector<task_index_t>& handles)
{
    bool all_completed = true;

    for (const task_index_t& handle : handles)
    {
        task_run_state state = get_task_run_state(handle);
        db_assert(state != task_run_state::unallocated);

        if (state != task_run_state::complete)
        {
            all_completed = false;
            break;
        }
    }

    return all_completed;
}

void task_scheduler::wait_for_tasks_no_help(const std::vector<task_index_t>& handles)
{
    std::unique_lock lock(m_task_mutex);

    while (true)
    {
        if (are_tasks_complete(handles))
        {
            return;
        }
        
        m_task_changed_convar.wait(lock);
    }
}

void task_scheduler::wait_for_tasks_helping(const std::vector<task_index_t>& handles)
{
    // Only help with queues that the tasks we are waiting for are contained within.
    // Saves us picking up some long running tasks from a loading queue, while waiting
    // for a tiny task on the stnadard queue.
    std::unordered_set<queue_state*> help_queues;
    
    for (const task_index_t& handle : handles)
    {
        task_state& state = get_task_state(handle);
        help_queues.insert(&m_queues[static_cast<int>(state.queue)]);
    }

    while (true)
    {
        task_index_t task;

        {
            // Check if any tasks are complete.
            std::unique_lock lock(m_task_mutex);
            if (are_tasks_complete(handles))
            {
                return;
            }

            // Otherwise try to pick up existing work.
            task = find_work(help_queues);
            if (task == k_invalid_task_index)
            {
                m_task_changed_convar.wait(lock);
            }
        }

        // Run the task we are helping with.
        if (task != k_invalid_task_index)
        {
            run_task(task);
        }
    }
}

task_scheduler::task_state& task_scheduler::get_task_state(task_index_t index)
{
    return m_tasks[index];
}

task_index_t task_scheduler::find_work(std::unordered_set<queue_state*>& queues)
{
    for (queue_state* queue : queues)
    {
        std::unique_lock lock(queue->work_mutex);
        if (!queue->work.empty())
        {
            task_index_t task_index = queue->work.front();
            queue->work.pop();
            return task_index;
        }
    }

    return k_invalid_task_index;
}

void task_scheduler::run_task(task_index_t task_index)
{
    task_state& state = m_tasks[task_index];
    db_assert(state.state == task_run_state::pending_run);

    state.state = task_run_state::running;
    state.work();

    // Reduce dependent counts and dispatch if they hit zero.
    for (task_index_t dependent_index : state.dependents)
    {
        task_state& dependent_state = m_tasks[dependent_index];        
        if (dependent_state.outstanding_dependencies.fetch_sub(1) == 1)            
        {
            std::scoped_lock lock(m_task_dispatch_mutex);

            // May not have been dispatched yet, dispatch will handle in this case.            
            if (dependent_state.state == task_run_state::pending_dependencies) 
            {
                dispatch_task(dependent_index);
            }
        }
    }

    // Complete task and notify anyone waiting.
    {
        std::unique_lock lock(m_task_mutex);
        state.state = task_run_state::complete;
        try_release_task_lockless(task_index);

        m_task_changed_convar.notify_all();
    }
}

void task_scheduler::worker_entry(task_scheduler::worker_state& state)
{
    while (!m_shutting_down)
    {
        while (!m_shutting_down)
        {
            task_index_t task_index = find_work(state.queues);
            if (task_index != k_invalid_task_index)
            {
                run_task(task_index);
            }
            else
            {
                break;
            }
        }

        state.work_sempahore.acquire();
    }
}

}; // namespace workshop
