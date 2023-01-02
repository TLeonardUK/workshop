// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"
#include "workshop.core/utils/frame_time.h"
#include "workshop.core/containers/command_queue.h"

namespace ws {

class render_command_queue;

// ================================================================================================
//  Represents all state that needs to be passed into the renderer to advance the rende rframe.
// ================================================================================================
class render_world_state
{
public:

    // The delta/frame-timing for the frame this state refers to.
    frame_time time;

    // Command queue that contains all the gameplay frames render commands.
    // Note: This is auto-populated by renderer::step.
    render_command_queue* command_queue;

};

}; // namespace ws
