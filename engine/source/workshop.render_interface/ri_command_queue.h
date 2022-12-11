// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

namespace ws {

class ri_command_list;

// ================================================================================================
//  Represents a single queue of execution on the gpu.
// ================================================================================================
class ri_command_queue
{
public:

    // Allocates a new command list which can be submitted to this queue.
    // This list is valid for the current frame.
    virtual ri_command_list& alloc_command_list() = 0;

    // Inserts a command list for execution on this queue.
    virtual void execute(ri_command_list& list) = 0;

};

}; // namespace ws
