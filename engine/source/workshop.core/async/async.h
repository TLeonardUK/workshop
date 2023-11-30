// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/async/task_scheduler.h"
#include "workshop.core/perf/profile.h"

namespace ws {

// ================================================================================================
//  Runs a task asyncronously in the task_scheduler worker pool.
//  This is essentially syntax suger to make async code a little less bloated.
// ================================================================================================
task_handle async(const char* name, task_queue queue, task_scheduler::task_function work);

// ================================================================================================
//  A for loop that runs in parallel. Executing different blocks of the loop on different workers.
//  Work is expected to be homogenous to spread the execution optimally.
// ================================================================================================
template <typename work_t>
void parallel_for(const char* name, task_queue queue, size_t count, work_t work, bool do_not_chunk = false, bool can_help_while_waiting = true)
{
    profile_marker(profile_colors::task, "%s [parallel]", name);

    if (count == 0)
    {
        return;
    }

    constexpr size_t k_chunks_per_task = 2;

    task_scheduler& scheduler = task_scheduler::get();

    size_t worker_count = scheduler.get_worker_count(queue);
    size_t task_count = std::min(worker_count, count);
    size_t chunk_size = std::max(1llu, count / task_count / k_chunks_per_task);

    // For very small amount of threads allocate them individually so every worker gets something.
    if (do_not_chunk || count < worker_count * 2)
    {
        chunk_size = 1;
    }

    std::atomic_size_t next_chunk = 0;

    std::vector<task_handle> handles = scheduler.create_tasks(task_count, name, queue, [&next_chunk, &work, chunk_size, count]() {

        while (true)
        {
            size_t range_start = next_chunk.fetch_add(chunk_size);
            if (range_start >= count)
            {
                break;
            }

            size_t remaining = count - range_start;
            size_t range_length = std::min(chunk_size, remaining);

            // TODO: Might just want to pass the range into the work as a parameter
            //       less overhead.
            for (size_t i = 0; i < range_length; i++)
            {
                work(range_start + i);
            }
        }

    });

    // Dispatch all tasks.
    {
        profile_marker(profile_colors::task, "dispatch tasks");
        scheduler.dispatch_tasks(handles);
    }

    // Wait for all tasks to complete.
    {
        profile_marker(profile_colors::task, "waiting for tasks");
        scheduler.wait_for_tasks(handles, can_help_while_waiting);
    }
}

}; // namespace workshop
