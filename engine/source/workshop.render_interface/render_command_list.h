// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

namespace ws {

class render_target;
class color;

// Describes the current access-state of a resource on the gpu.
enum class render_resource_state
{
    // Resource is usable as a render target.
    render_target,

    // Resource is only usable for presentation.
    present,
};

// ================================================================================================
//  Represents a list of commands that can be sent to a command_queue to execute.
// ================================================================================================
class render_command_list
{
public:

    // Called before recording command to this list.
    virtual void open() = 0;

    // Called after recording command to this list. The list is 
    // considered immutable after this call.
    virtual void close() = 0;

    // set targets
    // 

    // Inserts a resource barrier.
    virtual void barrier(render_target& resource, render_resource_state source_state, render_resource_state destination_state) = 0;

    // Clears a render target to a specific color.
    virtual void clear(render_target& resource, const color& destination) = 0;

    // Changes the rendering pipeline state.
    //virtual void set_pipeline(pipeline_state& state);

};

}; // namespace ws
