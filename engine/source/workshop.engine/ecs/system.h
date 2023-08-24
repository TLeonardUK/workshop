// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/frame_time.h"
#include "workshop.core/containers/command_queue.h"
#include "workshop.engine/ecs/object.h"
#include <typeindex>

namespace ws {

class object_manager;
class component;

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

    // Notifies the system that a component has been added to a given object so it can 
    // do any required setup.
    virtual void component_added(object handle, component* comp) {}

    // Notifies the system that a component has been removed from a given object so it can 
    // do any required teardown.
    virtual void component_removed(object handle, component* comp) {}

protected:

    // Runs all commands currently in the systems command queue. Should be called at least
    // once a frame to avoid it building up.
    void flush_command_queue();

    // Adds a dependency to another system. This system will not be stepped until 
    // all dependencies have completed their stepping.
    void add_dependency(const std::type_index& type_info, bool predecessor);

    template <typename system_type>
    void add_predecessor()
    {
        add_dependency(typeid(system_type), true);
    }

    template <typename system_type>
    void add_successor()
    {
        add_dependency(typeid(system_type), false);
    }

protected:

    static inline constexpr size_t k_command_queue_capacity = 4 * 1024 * 1024;

    object_manager& m_manager;

    std::vector<system*> m_dependencies;

    std::string m_name;

    command_queue m_command_queue;

};

}; // namespace ws
