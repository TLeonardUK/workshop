// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.physics_interface.jolt/jolt_pi_job_system.h"
#include "workshop.core/async/task_scheduler.h"

namespace ws {

int	jolt_pi_job_system::GetMaxConcurrency() const
{
    return (int)task_scheduler::get().get_worker_count(task_queue::standard);
}

jolt_pi_job_system::JobHandle jolt_pi_job_system::CreateJob(const char* inName, JPH::ColorArg inColor, const JobFunction& inJobFunction, JPH::uint32 inNumDependencies)
{
    // TODO
    return (JobHandle)0;
}

void jolt_pi_job_system::QueueJob(jolt_pi_job_system::Job* inJob)
{
    // TODO
}

void jolt_pi_job_system::QueueJobs(jolt_pi_job_system::Job** inJobs, JPH::uint inNumJobs)
{
    // TODO
}

void jolt_pi_job_system::FreeJob(jolt_pi_job_system::Job* inJob)
{
    // TODO
}

}; // namespace ws
