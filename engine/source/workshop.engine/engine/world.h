// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/frame_time.h"
#include "workshop.engine/ecs/object_manager.h"

#include <string>
#include <memory>

namespace ws {

    class engine;

// ================================================================================================
//  Each world class represents an individual "universe", with its own set of objects 
//  and attributes. 
// ================================================================================================
class world
{
public:

    world(engine& engine);

    // Gets a descriptive name of this world.
    const char* get_name();

    // Gets the manager that handles constructing/destroying objects and their associated components.
    object_manager& get_object_manager();

    // Called once each frame, steps the world.
    void step(const frame_time& time);

    // Gets the engine that owns this world.
    engine& get_engine();

protected:

private:
    std::string m_name;

    std::unique_ptr<object_manager> m_object_manager;

    engine& m_engine;

};

}; // namespace ws
