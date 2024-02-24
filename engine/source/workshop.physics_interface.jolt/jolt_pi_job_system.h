// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Core/Core.h>
#include <Jolt/Core/JobSystemWithBarrier.h>

namespace ws {

// ================================================================================================
//  Implementation of jolt job system that integrated when the engines job system.
// ================================================================================================
class jolt_pi_job_system : public JPH::JobSystemWithBarrier
{
public:
    virtual int	GetMaxConcurrency() const override;
    virtual JobHandle CreateJob(const char* inName, JPH::ColorArg inColor, const JobFunction& inJobFunction, JPH::uint32 inNumDependencies = 0);

protected:
    virtual void QueueJob(Job* inJob) override;
    virtual void QueueJobs(Job** inJobs, JPH::uint inNumJobs) override;
    virtual void FreeJob(Job* inJob) override;

};

}; // namespace ws
