// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/frame_time.h"
#include <typeindex>

namespace ws {

class object_manager;

// ================================================================================================
//  Base class for all systems.
// 
//  Systems are responsible for taking a set of related objects and performing logical
//  operations on them.
// ================================================================================================
class system
{
public:

    system(object_manager& manager, const char* name);

    // Gets a debugging name for this system.
    const char* get_name();

    // Called once each frame, steps the system by one frame.
    virtual void step(const frame_time& time) = 0;

    // Gets a list of systems that need to be ticked before this one.
    std::vector<system*> get_dependencies();

protected:

    // Adds a dependency to another system. This system will not be stepped until 
    // all dependencies have completed their stepping.
    void add_dependency(const std::type_index& type_info);

    template <typename system_type>
    void add_dependency()
    {
        add_dependency(typeid(system_type));
    }

protected:

    object_manager& m_manager;

    std::vector<system*> m_dependencies;

    std::string m_name;

};

}; // namespace ws