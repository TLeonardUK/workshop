// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/async/async.h"
#include "workshop.core/async/task_scheduler.h"

namespace ws {

task_handle async(const char* name, task_queue queue, task_scheduler::task_function work)
{
    task_handle handle = task_scheduler::get().create_task(name, queue, work);
    handle.dispatch();
    return handle;
}

}; // namespace workshop
