// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"
#include "workshop.core/utils/frame_time.h"

#include "workshop.renderer/render_view.h"

namespace ws {

// ================================================================================================
//  Represents the state of all renderable objects at a given point in time. 
//  This is filled in by the main thread and passed to the renderer to being rendering the world
//  in parallel.
// ================================================================================================
class render_world_state
{
public:

    frame_time time;

    std::vector<render_view> views;

    //std::vector<render_scene_object> scene_objects;

};

}; // namespace ws
