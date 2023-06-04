// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

namespace ws {

class ri_command_queue;

// ================================================================================================
//  Fence used for syncronization between the CPU and GPU.
// ================================================================================================
class ri_fence
{
public:
    virtual ~ri_fence() {}

    // Blocks cpu until the fence has signaled the current value. 
    virtual void wait(size_t value) = 0;

    // Inserts a command into the graphics queue to wait for a given value.
    virtual void wait(ri_command_queue& queue, size_t value) = 0;

    // Gets the value of the most recent value signaled.
    virtual size_t current_value() = 0;

    // Increments the value of this fence to the new value from the cpu.
    virtual void signal(size_t value) = 0;

    // Inserts a command into the graphics queue to signal a given value.
    virtual void signal(ri_command_queue& queue, size_t value) = 0;

};

}; // namespace ws
