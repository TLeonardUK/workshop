// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/async/task_scheduler.h"

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
void parallel_for(const char* name, task_queue queue, size_t count, work_t work)
{
    constexpr size_t k_chunks_per_task = 2;

    task_scheduler& scheduler = task_scheduler::get();

    size_t task_count = std::min(scheduler.get_worker_count(queue), count);
    size_t chunk_size = 256;//std::max(1llu, count / task_count / k_chunks_per_task);

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
    scheduler.dispatch_tasks(handles);

    // Wait for all tasks to complete.
    scheduler.wait_for_tasks(handles);
}

}; // namespace workshop
