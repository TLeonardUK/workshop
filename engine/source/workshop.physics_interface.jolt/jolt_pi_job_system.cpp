// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.physics_interface.jolt/jolt_pi_job_system.h"
#include "workshop.core/async/task_scheduler.h"

namespace ws {

jolt_pi_job_system::jolt_pi_job_system()
    : JPH::JobSystemWithBarrier(k_max_barriers)
{
    // The lack of a default constructor for Job means we can't do this using std::array or 
    // a similar standard container :|
    m_jobs = (Job*)malloc(sizeof(Job) * k_max_jobs);

    m_free_job_indices.reserve(k_max_jobs);
    for (size_t i = 0; i < k_max_jobs; i++)
    {
        m_free_job_indices.push_back(i);
    }
}

jolt_pi_job_system::~jolt_pi_job_system()
{
    free(m_jobs);
}

int	jolt_pi_job_system::GetMaxConcurrency() const
{
    return (int)task_scheduler::get().get_worker_count(task_queue::standard);
}

jolt_pi_job_system::JobHandle jolt_pi_job_system::CreateJob(const char* inName, JPH::ColorArg inColor, const JobFunction& inJobFunction, JPH::uint32 inNumDependencies)
{
    size_t job_index = 0;
    for (;;)
    {
        {
            std::scoped_lock lock(m_job_mutex);
            if (m_free_job_indices.size() > 0)
            {
                job_index = m_free_job_indices.back();
                m_free_job_indices.pop_back();
                break;
            }
        }

        db_warning(core, "jolt_pi_job_system ran out of jobs, consider increasing k_max_jobs. Stalling until one is available.");
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    Job* job = m_jobs + job_index;
    ::new (job) Job(inName, inColor, this, inJobFunction, inNumDependencies);

    JobHandle handle(job);

    if (inNumDependencies)
    {
        QueueJob(job);
    }

    return handle;
}

void jolt_pi_job_system::QueueJob(jolt_pi_job_system::Job* inJob)
{
    task_scheduler& scheduler = task_scheduler::get();
    task_handle handle = scheduler.create_task("jolt physics", task_queue::standard, [inJob]() {
        inJob->Execute();
    });
    scheduler.dispatch_tasks({ handle });
}

void jolt_pi_job_system::QueueJobs(jolt_pi_job_system::Job** inJobs, JPH::uint inNumJobs)
{
    task_scheduler& scheduler = task_scheduler::get();

    std::vector<task_handle> handles;
    handles.resize(inNumJobs);

    for (size_t i = 0; i < inNumJobs; i++)
    {
        jolt_pi_job_system::Job* job = inJobs[i];

        handles[i] = scheduler.create_task("jolt physics", task_queue::standard, [job]() {
            job->Execute();
        });
    }

    scheduler.dispatch_tasks(handles);
}

void jolt_pi_job_system::FreeJob(jolt_pi_job_system::Job* inJob)
{
    std::scoped_lock lock(m_job_mutex);
    size_t index = std::distance(m_jobs, inJob);
    inJob->~Job();
    m_free_job_indices.push_back(index);
}

}; // namespace ws
