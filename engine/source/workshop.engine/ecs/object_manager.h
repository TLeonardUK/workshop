// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/frame_time.h"
#include "workshop.engine/ecs/object.h"
#include "workshop.engine/ecs/system.h"
#include <typeindex>
#include <unordered_map>

namespace ws {

class component;

// ================================================================================================
//  Responsible for the creation/destruction of objects and their associated components.
//  Behaviour of public functions are thread safe.
// ================================================================================================
class object_manager
{
public:

    // Called once each frame, steps all the systems.
    void step(const frame_time& time);

public:

    // Registers a system that will be updated as part of this world.
    template <typename system_type, typename ...args>
    void register_system(args... input)
    {
        std::scoped_lock lock(m_system_mutex);

        m_systems.push_back(std::make_unique<system_type>(*this, input...));
    }

    // Unregisters a system previously registered.
    template <typename system_type>
    void unregister_system()
    {
        std::scoped_lock lock(m_system_mutex);

        auto iter = std::find(m_systems.begin(), m_systems.end(), [](auto& sys) {
            return typeid(sys.get()) == typeid(system_type);
            });
        if (iter != m_systems.end())
        {
            m_systems.erase(iter);
        }
    }

    // Gets a system based on its type.
    system* get_system(const std::type_index& type_info);

    template <typename system_type>
    system_type* get_system()
    {
        return static_cast<system_type*>(get_system(typeid(system_type)));
    }

public:

    // Creates a new object and returns an opaque reference to it.
    object create_object();

    // Destroys an object previously created with create_object.
    void destroy_object(object obj);

    // Returns true if the given object is currently active.
    bool is_object_alive(object obj);

    // Add a component of the given type to the given object.
    template <typename component_type>
    component_type* add_component(object handle)
    {
        // TODO
        return nullptr;
    }

    // Removes the first component of the given type from the given object.
    template <typename component_type>
    void remove_component(object handle)
    {
        // TODO
    }

    // Removes the specific component from the given object..
    void remove_component(object handle, component* component);

    // Gets all components attached to the given object.
    std::vector<component*> get_components(object handle);

protected:
    // TODO: I do not like this, if m_objects gets rehashed this is 
    // potentially a fuck load of data to churn.
    struct object_state
    {
        object handle;
        std::unordered_map<std::type_index, uint64_t> m_components;
    };

    // Commits the creation/destruction of an object. This is either done
    // immediately in create_object/destroy_object or defered if a tick is active.
    void commit_destroy_object(object obj);

    // Updates which filters/etc this object is registered for.
    void update_object_registration(object_state& state);

    // Gets the state for a given object handle.
    object_state* get_object_state(object obj);

    void step_systems(const frame_time& time);

private:    
    std::recursive_mutex m_object_mutex;
    std::recursive_mutex m_system_mutex;

    std::vector<std::unique_ptr<system>> m_systems;

    uint64_t m_next_object_handle;
    std::unordered_map<object, object_state> m_objects;
    std::vector<object> m_pending_registration;
    std::vector<object> m_pending_destroy;

    bool m_is_system_step_active = false;

};


}; // namespace ws
