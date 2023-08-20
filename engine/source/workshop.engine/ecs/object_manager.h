// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/frame_time.h"
#include "workshop.core/containers/sparse_vector.h"
#include "workshop.engine/ecs/object.h"
#include "workshop.engine/ecs/system.h"
#include "workshop.engine/ecs/component.h"
#include "workshop.core/memory/memory_tracker.h"
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

    object_manager();

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

private:

    // Maximum number of objects that can exist at once. 
    // This does not imply that memory for all these objects will be created.
    static inline constexpr size_t k_max_objects = 1'000'000;

    // Maximum number of components of each type that can exist at once. 
    // This does not imply that memory for all these components will be created.
    static inline constexpr size_t k_max_components = 1'000'000;

    struct object_state
    {
        object handle;

        // Given sizes of component lists, linear searches are
        // faster than hash tables / etc.
        std::vector<component*> components;
    };

    class component_pool_base
    {
    public:
        virtual component* alloc() = 0;
        virtual void free(component* result) = 0;
    };

    template <typename component_type>
    class component_pool : public component_pool_base
    {
    public:
        component_pool()
            : m_storage(k_max_components)
        {
            // Always allocate the first index so we can assume 0=null.
            size_t index = m_storage.insert({});
            db_assert(index == 0);
        }

        virtual component* alloc() override
        {
            size_t index = m_storage.insert({});
            return &m_storage[index];
        }

        virtual void free(component* result) override
        {
            m_storage.remove(static_cast<component_type*>(result));
        }

    private:
        sparse_vector<component_type, memory_type::engine__ecs> m_storage;
    };

    // Gets the state for a given object handle.
    object_state* get_object_state(object obj);

    // Gets the component pool that components of the given type are allocated from.
    template <typename component_type>
    component_pool_base& get_component_pool()
    {
        std::type_index component_type_index = typeid(component_type);

        auto iter = m_component_pools.find(component_type_index);
        if (iter != m_component_pools.end())
        {
            return *iter->second;
        }

        std::unique_ptr<component_pool_base> pool = std::make_unique<component_pool<component_type>>();
        component_pool_base& ret = *pool.get();
        m_component_pools[component_type_index] = std::move(pool);
        return ret;
    }

    component_pool_base* get_component_pool(const std::type_index& component_type_index);

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
        component_pool_base& pool = get_component_pool<component_type>();
        component* comp = pool.alloc();
        add_component(handle, comp);
        return static_cast<component_type*>(comp);
    }

    // Adds the specific component from the given object.
    void add_component(object handle, component* component);

    // Removes the first component of the given type from the given object.
    template <typename component_type>
    void remove_component(object handle)
    {
        object_state* state = get_object_state(handle);
        if (!state)
        {
            return;
        }

        std::type_index search_comp_type_index = typeid(component_type);
        auto iter = std::find_if(state->components.begin(), state->components.end(), [search_comp_type_index](auto& comp) {
            std::type_index comp_type_index = typeid(*comp);
            return comp_type_index == search_comp_type_index;
        });

        if (iter != state->components.end())
        {
            remove_component(handle, *iter);
        }
    }

    // Removes the specific component from the given object..
    void remove_component(object handle, component* component);

    // Gets all components attached to the given object.
    std::vector<component*> get_components(object handle);

protected:

    // Commits the creation/destruction of an object. This is either done
    // immediately in create_object/destroy_object or defered if a tick is active.
    void commit_destroy_object(object obj);

    // Commits the removal of a component. This is either done
    // immediately in remove_component or defered if a tick is active.
    void commit_remove_component(object obj, component* component);

    // Updates which filters/etc this object is registered for.
    void update_object_registration(object_state& state);

    void step_systems(const frame_time& time);

private:    
    std::recursive_mutex m_object_mutex;
    std::recursive_mutex m_system_mutex;

    std::vector<std::unique_ptr<system>> m_systems;

    sparse_vector<object_state, memory_type::engine__ecs> m_objects;
    std::vector<std::pair<object, component*>> m_pending_remove_component;
    std::vector<object> m_pending_registration;
    std::vector<object> m_pending_destroy;

    std::unordered_map<std::type_index, std::unique_ptr<component_pool_base>> m_component_pools;

    bool m_is_system_step_active = false;

};


}; // namespace ws
