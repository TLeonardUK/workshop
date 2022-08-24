// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

namespace ws {

class render_command_list;

// ================================================================================================
//  Represents a single queue of execution on the gpu.
// ================================================================================================
class render_command_queue
{
public:

    // Allocates a new command list which can be submitted to this queue.
    // This list is valid for the current frame.
    virtual render_command_list& alloc_command_list() = 0;

    // Inserts a command list for execution on this queue.
    virtual void execute(render_command_list& list) = 0;

};

}; // namespace ws
