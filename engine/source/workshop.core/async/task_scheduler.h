// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/singleton.h"

#include <thread>
#include <string>
#include <vector>
#include <array>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <semaphore>
#include <unordered_set>

namespace ws {

class task_scheduler;

using task_index_t = size_t;

// Identifies the different queues a task can be in. Different
// workers will prioritize different queues to balance workload.
enum class task_queue
{
    standard,
    loading,

    COUNT
};

// ================================================================================================
//  Holds a reference to a task that has been previously created
//  by the task scheduler. Can be used to query the current state.
// 
//  Handles should be destroyed when keeping track of the task is no longer
//  neccessary. Failing to do this will run the scheduler out of valid
//  tasks as tasks with references will not be recycled.
// ================================================================================================
struct task_handle
{
public:
    task_handle();
    task_handle(task_scheduler* scheduler, task_index_t index);
    task_handle(task_handle&& other);
    task_handle(const task_handle& other);
    ~task_handle();

    // Determines if this handle points to a valid handle.
    bool is_valid();

    // Determines if the task this handle points to has completed.
    bool is_complete();

    // Determines if the task has been dispatched yet.
    bool is_dispatched();

    // Adds a dependency which will have to execute first before 
    // the task this points to can execute.
    // 
    // Must be called before dispatching task.
    void add_dependency(const task_handle& other);

    // Dispatches this task, queuing it for execution.
    void dispatch();

    // Blocks until this task has completed.
    void wait(bool can_help = false);

    // Resets this handle so it no longer points at a valid task.
    void reset();

    // Generatel operation overloads.
    task_handle& operator=(const task_handle& other);
    task_handle& operator=(task_handle&& other);

    bool operator==(task_handle const& other) const;
    bool operator!=(task_handle const& other) const;

protected:
    void increment_ref();
    void decrement_ref();

    task_index_t get_task_index() const;

private:
    friend class task_scheduler;

    task_scheduler* m_task_scheduler;
    task_index_t m_index;

};

// ================================================================================================
//  This is the task scheduler. 
// 
//  It allows the creation of tasks which are automatically run asyncronously by 
//  on worker threads.
// 
//  Tasks can have dependencies to ensure ordered-execution.
// 
//  Tasks are placed into several queues, depending on the task_queue type they are created with. 
//  Different workers will prioritize different queues to balance workload. This prevents issues
//  such as batch loading assets saturating all available cores.
// ================================================================================================
class task_scheduler 
    : public singleton<task_scheduler>
{
public:

    static inline constexpr size_t k_invalid_task_index = std::numeric_limits<size_t>::max();

    using task_function = std::function<void()>;

    struct init_state
    {
        // How many workers should be created. In general you want this to equal
        // the number of threads on the processor.
        size_t worker_count = 0;

        // Determines how many workers are allow to run tasks from each queue.
        //  1.0f = All workers can run tasks from the queue.
        //  0.0f = A single worker can run tasks from the queue (minimum is always one worker).
        std::array<float, static_cast<int>(task_queue::COUNT)> queue_weights = { 1.0f };
    };
    
    task_scheduler(init_state& states);
    ~task_scheduler();

    // Gets number of workers that can process the given queue.
    size_t get_worker_count(task_queue queue);

    // Creates a new task that is placed in the given task queue. The task will not
    // start running until dispatch is called on it.
    task_handle create_task(const char* name, task_queue queue, task_function workload);

    // Same as create_task but can create multiple at once to reduce overhead.
    std::vector<task_handle> create_tasks(size_t count, const char* name, task_queue queue, task_function workload);

    // Blocks until all pending tasks have completed.
    void drain();

    // Waits for all the given tasks, useful to reduce overhead.
    void wait_for_tasks(const std::vector<task_handle>& handles, bool can_help = false);

    // Dispatches multiple tasks at once, useful to reduce overhead.
    void dispatch_tasks(const std::vector<task_handle>& handles);

protected:

    friend struct task_handle;

    enum class task_run_state
    {
        unallocated,
        pending_dispatch,
        pending_dependencies,
        pending_run,
        running,
        complete
    };

    struct task_state
    {
        size_t index = 0;
        std::atomic_size_t references = 0;
        task_queue queue = task_queue::standard;
        task_run_state state = task_run_state::unallocated;
        task_function work;

        std::vector<task_index_t> dependents;
        std::atomic_size_t outstanding_dependencies = 0;

        char name[128];
    };

    struct queue_state
    {
        std::queue<task_index_t> work;
        std::mutex work_mutex;
        size_t worker_count;
    };

    struct worker_state
    {
        worker_state()
            : work_sempahore(0)
        {
        }

        worker_state(worker_state&& other)
            : work_sempahore(0)
            , queues(other.queues)
            , thread(std::move(other.thread))
        {
        }

        std::unordered_set<queue_state*> queues;
        std::unique_ptr<std::thread> thread;
        std::counting_semaphore<std::numeric_limits<uint32_t>::max()> work_sempahore;
    };

    // Waits for the given task.
    void wait_for_task(task_index_t index, bool can_help = false);
    void wait_for_tasks_no_help(const std::vector<task_index_t>& handles);
    void wait_for_tasks_helping(const std::vector<task_index_t>& handles);

    // Determines if a list of tasks are all completed yet.
    bool are_tasks_complete(const std::vector<task_index_t>& handles);

    // Gets the task state from the given index.
    task_state& get_task_state(task_index_t index);

    // Core function all workers threads execute.
    void worker_entry(worker_state& state);

    // Tries to free a task. Will succeed when refcount is 0
    // and task has finished running.
    void try_release_task(task_index_t index);
    void try_release_task_lockless(task_index_t index);

    // Returns if the given task index has completed.
    task_run_state get_task_run_state(task_index_t task_index);

    // Adds another task as a dependency of another task.
    // The dependent task will always run first.
    void add_task_dependency(task_index_t task_index, task_index_t dependency_index);

    // Queues the task so it can be picked up and run.
    void dispatch_task(task_index_t index);
    void dispatch_tasks_lockless(const std::vector<task_index_t>& indices);

private:

    // Allocates the next free task index in the m_tasks 
    // list. Asserts on failure.
    task_index_t alloc_task_index();
    task_index_t alloc_task_index_lockless();

    // Releases a task index previously allocated by
    // alloc_task_index.
    void free_task_index(task_index_t task_index);

    // Finds a task the given worker state can execute and 
    // pops it from the queue. The worker is expected to
    // run this task on return.
    task_index_t find_work(std::unordered_set<queue_state*>& queues);

    // Executes the given task index.
    void run_task(task_index_t task_index);

private:

    inline static constexpr size_t k_max_tasks = 1024 * 64;

    std::mutex m_task_allocation_mutex;
    std::array<task_state, k_max_tasks> m_tasks;
    std::queue<size_t> m_free_task_indices;

    std::recursive_mutex m_task_dispatch_mutex;

    std::array<queue_state, static_cast<int>(task_queue::COUNT)> m_queues;
    std::vector<worker_state> m_workers;

    std::atomic<bool> m_shutting_down = false;

    std::condition_variable m_task_changed_convar;
    std::mutex m_task_mutex;

};

}; // namespace workshop
