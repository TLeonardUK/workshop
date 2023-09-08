// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/frame_time.h"
#include "workshop.core/containers/sparse_vector.h"
#include "workshop.engine/ecs/object.h"
#include "workshop.engine/ecs/system.h"
//#include "workshop.engine/ecs/component.h"
#include "workshop.engine/ecs/component_filter_archetype.h"
#include "workshop.core/memory/memory_tracker.h"
#include "workshop.core/hashing/hash.h"
#include <typeindex>
#include <unordered_map>

namespace ws {

class component;
class component_filter_archetype;

// Simple wrapper for a set of component types, used as a key for associative containers.
struct component_types_key
{
    std::vector<std::type_index> include_component_types;
    std::vector<std::type_index> exclude_component_types;

    size_t get_hash() const
    {
        std::size_t h = 0;
        for (size_t i = 0; i < include_component_types.size(); i++)
        {
            hash_combine(h, include_component_types[i]);
        }
        for (size_t i = 0; i < exclude_component_types.size(); i++)
        {
            hash_combine(h, exclude_component_types[i]);
        }
        return h;
    }

    bool operator==(const component_types_key& other) const
    {
        return std::equal(include_component_types.begin(), include_component_types.end(), other.include_component_types.begin(), other.include_component_types.end()) &&
               std::equal(exclude_component_types.begin(), exclude_component_types.end(), other.exclude_component_types.begin(), other.exclude_component_types.end());
    }

    bool operator!=(const component_types_key& other) const
    {
        return !operator==(other);
    }
};

}; // namespace ws

// ================================================================================================
// Specialization of object_manager::component_filter_archetype_key for guid so we can use it in maps/etc.
// ================================================================================================
template<>
struct std::hash<ws::component_types_key>
{
    std::size_t operator()(const ws::component_types_key& k) const
    {
        return k.get_hash();
    }
};

namespace ws {

class world;

// ================================================================================================
//  Responsible for the creation/destruction of objects and their associated components.
//  Behaviour of public functions are thread safe.
// ================================================================================================
class object_manager
{
public:

    // Maximum number of objects that can exist at once. 
    // This does not imply that memory for all these objects will be created.
    static inline constexpr size_t k_max_objects = 1'000'000;

    // Maximum number of components of each type that can exist at once. 
    // This does not imply that memory for all these components will be created.
    static inline constexpr size_t k_max_components = 1'000'000;

    object_manager(world& world);
    ~object_manager();

    // Called once each frame, steps all the systems.
    void step(const frame_time& time);

    // Gets the world this object manager is owned by.
    world& get_world();

public:

    // Registers a component type with the object manager. 
    // This just ensures pools/etc are setup, there is no need to handle unregistering.
    template <typename component_type>
    void register_component()
    {
        // Ensure the pool is setup for this component.
        get_component_pool<component_type>();
    }

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

    // Gets a filter archetype for the given set of component types. Generally
    // there isn't a good reason to use this directly, use a component_filter instead.
    component_filter_archetype* get_filter_archetype(const std::vector<std::type_index>& include_components, const std::vector<std::type_index>& exclude_components);

    // Invoked when a reflected field of a component has been modified and the systems
    // that use it need to be updated.
    // 
    // In generaly this SHOULD NEVER be used in user-code, its here to support property-modification
    // in the editor. User code should send messages to systems for them to modify a component, not
    // do it directly.
    void component_edited(object obj, component* comp);

    // Same behaviour as component_editor but applies to all components attached to the given object.
    void object_edited(object obj);

    // This is equivilent to calling component_edited on every single component that exists. 
    //
    // DO NOT CALL THIS.
    // Its purpose is to force systems to update their view of components after an entire scene has 
    // been deserialized. It is expensive to perform and unneccessary in mostly any other situation.
    void all_components_edited();

    // Gets a list of all alive objects.
    //
    // This is very expensive to generate, and outside of serialization this is a very suspicious function
    // to be calling. Consider if the best option over using a filter.
    std::vector<object> get_objects();

private:    

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

        // TODO: Allocate the same index as the object handle, this would keep
        // all components for each object in consecutive addresses. Better cache efficiency
        // and no need to do sorting in filters. Linear accessing when trying to get inidividual
        // object ids as well.

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
    object create_object(const char* name);

    // Creates an object with the specific handle. Will assert if handle
    // is already allocated.
    // 
    // Unlike the standard create_object this function will not create meta
    // components as these are expected to be created by the caller.
    // 
    // This function has a very specific use for deserializing object states, 
    // it's not generally something that should be used outside of that.
    object create_object(object handle);

    // Same as above, but still creates the meta components.
    object create_object(const char* name, object handle);

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

    // Add a component of the given type to the given object.
    component* add_component(object handle, std::type_index index);

    // Adds the specific component from the given object.
    void add_component(object handle, component* component);

    // Removes the first component of the given type from the given object.
    template <typename component_type>
    void remove_component(object handle)
    {
        std::scoped_lock lock(m_object_mutex);

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

    // Removes a component of the given type to the given object.
    void remove_component(object handle, std::type_index index);

    // Gets all components attached to the given object.
    std::vector<component*> get_components(object handle);

    // Gets all components attached to the given object.
    std::vector<std::type_index> get_component_types(object handle);
    
    // Gets the first component of the given type from the given object.
    template <typename component_type>
    component_type* get_component(object handle)
    {
        std::scoped_lock lock(m_object_mutex);

        object_state* state = get_object_state(handle);
        if (!state)
        {
            return nullptr;
        }

        //std::type_index search_comp_type_index = typeid(component_type);
        for (auto& comp : state->components)
        {
            if (component_type* ret = dynamic_cast<component_type*>(comp); ret != nullptr)
            {
                return ret;
            }
        }

        return nullptr;
    }

    // Gets a component from the given object with the given type.
    component* get_component(object handle, std::type_index index);

    // Serializes an objects component to binary, the component
    // is untouched. This can be used to store the state of an component temporarily.
    std::vector<uint8_t> serialize_component(object handle, std::type_index component_type);

    // Deserializes the state of a component that was serialized by serialize_component.
    // If a component of the same type on the object exists, it is stomped over.
    void deserialize_component(object handle, const std::vector<uint8_t>& data, bool mark_as_edited = true);

    // Serializes an object and its components to binary, the object
    // is untouched. This can be used to store the state of an object temporarily.
    std::vector<uint8_t> serialize_object(object handle);

    // Deserializes the state of an object that was serialized by serialize_object.
    // Any components existing on the object that are not in the serialized state
    // will be removed.
    void deserialize_object(object handle, const std::vector<uint8_t>& data, bool mark_as_edited = true);

protected:

    // Commits the creation/destruction of an object. This is either done
    // immediately in create_object/destroy_object or defered if a tick is active.
    void commit_destroy_object(object obj);

    // Commits the removal of a component. This is either done
    // immediately in remove_component or defered if a tick is active.
    void commit_remove_component(object obj, component* component, bool update_registration);

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

    std::unordered_map<component_types_key, std::unique_ptr<component_filter_archetype>> m_component_filter_archetype;

    bool m_is_system_step_active = false;

    world& m_world;

};

}; // namespace ws
