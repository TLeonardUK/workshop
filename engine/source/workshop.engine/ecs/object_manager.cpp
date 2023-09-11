// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/ecs/object_manager.h"
#include "workshop.engine/ecs/component_filter_archetype.h"
#include "workshop.engine/ecs/component.h"
#include "workshop.engine/ecs/meta_component.h"
#include "workshop.core/async/task_scheduler.h"
#include "workshop.core/perf/profile.h"
#include "workshop.core/filesystem/ram_stream.h"

namespace ws {

object_manager::object_manager(world& world)
    : m_objects(k_max_objects)
    , m_world(world)
{
    // Always allocate the first index so we can assume 0=null.
    size_t index = m_objects.insert({});
    db_assert(index == 0);
}

object_manager::~object_manager()
{
    // Make sure to destroy all objects so systems have a chance to remove anything 
    // outside of the object system (render objects, etc).

    // This is a shit way to do this, we need to store all active objects in a better way.
    for (size_t i = 1; i < m_objects.capacity(); i++)
    {
        if (m_objects.is_valid(i))
        {
            destroy_object(i);
        }
    }

    // Flush all systems command queues to drain out any defered deletions.
    std::scoped_lock lock(m_system_mutex);
    for (size_t i = 0; i < m_systems.size(); i++)
    {
        auto& system = m_systems[i];
        system->flush_command_queue();
    }
}

world& object_manager::get_world()
{
    return m_world;
}

std::vector<object> object_manager::get_objects()
{
    std::vector<object> result;

    for (size_t i = 1; i < m_objects.capacity(); i++)
    {
        if (m_objects.is_valid(i))
        {
            result.push_back(i);
        }
    }

    return result;
}

system* object_manager::get_system(const std::type_index& type_info)
{
    std::scoped_lock lock(m_system_mutex);

    auto iter = std::find_if(m_systems.begin(), m_systems.end(), [&type_info](const std::unique_ptr<system>& sys) {
        std::type_index sys_index = typeid(*sys.get());
        return sys_index == type_info;
    });

    if (iter != m_systems.end())
    {
        return iter->get();
    }

    return nullptr;
}

object object_manager::create_object(const char* name)
{
    std::scoped_lock lock(m_object_mutex);

    size_t index = m_objects.insert({});
    object_state& state = m_objects[index];
    state.handle = index;

    // Add the meta component that should always exist.
    meta_component* meta = add_component<meta_component>(state.handle);
    meta->name = name;

    return state.handle;
}

object object_manager::create_object(object handle)
{
    std::scoped_lock lock(m_object_mutex);

    size_t index = m_objects.insert(handle, {});
    object_state& state = m_objects[index];
    state.handle = index;

    return state.handle;
}

object object_manager::create_object(const char* name, object handle)
{
    std::scoped_lock lock(m_object_mutex);

    size_t index = m_objects.insert(handle, {});
    object_state& state = m_objects[index];
    state.handle = index;

    // Add the meta component that should always exist.
    meta_component* meta = add_component<meta_component>(state.handle);
    meta->name = name;

    return state.handle;
}

void object_manager::destroy_object(object obj)
{
    std::scoped_lock lock(m_object_mutex);

    if (m_is_system_step_active)
    {
        m_pending_destroy.push_back(obj);
    }
    else
    {
        commit_destroy_object(obj);
    }
}

object_manager::component_pool_base* object_manager::get_component_pool(const std::type_index& component_type_index)
{
    auto iter = m_component_pools.find(component_type_index);
    if (iter != m_component_pools.end())
    {
        return iter->second.get();
    }

    return nullptr;
}

component_filter_archetype* object_manager::get_filter_archetype(const std::vector<std::type_index>& include_components_unsorted, const std::vector<std::type_index>& exclude_components_unsorted)
{
    std::scoped_lock lock(m_object_mutex);

    // Sort into a deterministic order so filters that just have an order difference
    // don't create an entirely new archetype.
    std::vector<std::type_index> include_component_types = include_components_unsorted;
    std::vector<std::type_index> exclude_component_types = exclude_components_unsorted;
    std::sort(include_component_types.begin(), include_component_types.end());
    std::sort(exclude_component_types.begin(), exclude_component_types.end());

    component_types_key key;
    key.include_component_types = include_component_types;
    key.exclude_component_types = exclude_component_types;

    auto iter = m_component_filter_archetype.find(key);
    if (iter != m_component_filter_archetype.end())
    {
        return iter->second.get();
    }

    std::unique_ptr<component_filter_archetype> archetype = std::make_unique<component_filter_archetype>(*this, include_component_types, exclude_component_types);
    component_filter_archetype* ret = archetype.get();
    m_component_filter_archetype.emplace(key, std::move(archetype));

    // All all objects to the archetype.
    // Start at 1 to skip the 0=null index.
    // This is a shit way to do this, we need to store all active objects in a better way. But it should
    // only happen once per filter type, so tolerable for now.
    for (size_t i = 1; i < m_objects.capacity(); i++)
    {
        if (m_objects.is_valid(i))
        {
            ret->update_object(i);
        }
    }

    return ret;
}

void object_manager::component_edited(object obj, component* comp, component_modification_source source)
{
    std::scoped_lock lock(m_system_mutex);
    for (size_t i = 0; i < m_systems.size(); i++)
    {
        auto& system = m_systems[i];
        system->component_modified(obj, comp, source);
    }
}

void object_manager::object_edited(object obj, component_modification_source source)
{
    std::scoped_lock obj_lock(m_object_mutex);
    std::scoped_lock sys_lock(m_system_mutex);

    object_manager::object_state* state = get_object_state(obj);
    if (!state)
    {
        return;
    }

    for (component* comp : state->components)
    {
        for (size_t i = 0; i < m_systems.size(); i++)
        {
            auto& system = m_systems[i];
            system->component_modified(obj, comp, source);
        }
    }
}

void object_manager::all_components_edited(component_modification_source source)
{
    std::scoped_lock lock(m_system_mutex);

    // TODO: Add a better way to get all alive objects.
    for (size_t i = 1; i < m_objects.capacity(); i++)
    {
        object_manager::object_state* state = get_object_state(i);
        if (state)
        {
            for (component* comp : state->components)
            {
                for (size_t j = 0; j < m_systems.size(); j++)
                {
                    auto& system = m_systems[j];
                    system->component_modified(i, comp, source);
                }
            }
        }
    }
}

void object_manager::commit_destroy_object(object handle)
{
    object_manager::object_state* state = get_object_state(handle);
    if (!state)
    {
        return;
    }

    std::vector<component*> to_remove = state->components;
    for (component* comp : to_remove)
    {
        commit_remove_component(handle, comp, false);
    }

    for (auto& pair : m_component_filter_archetype)
    {
        pair.second->remove_object(handle);
    }

    m_objects.remove(handle);
}

void object_manager::commit_remove_component(object handle, component* comp, bool update_registration)
{
    object_manager::object_state* state = get_object_state(handle);
    if (!state)
    {
        return;
    }

    auto iter = std::find(state->components.begin(), state->components.end(), comp);
    if (iter != state->components.end())
    {
        // Let systems know the component is being freed to sit can clean up anything if needed.
        {
            std::scoped_lock lock(m_system_mutex);
            for (size_t i = 0; i < m_systems.size(); i++)
            {
                auto& system = m_systems[i];
                system->component_removed(handle, comp);
            }
        }

        component_pool_base* base = get_component_pool(typeid(*comp));
        base->free(comp);
        state->components.erase(iter);
    }

    if (update_registration)
    {
        update_object_registration(*state);
    }
}

void object_manager::update_object_registration(object_state& obj)
{
    // Defer till end of tick.
    if (m_is_system_step_active)
    {
        m_pending_registration.push_back(obj.handle);
        return;
    }

    for (auto& pair : m_component_filter_archetype)
    {
        pair.second->update_object(obj.handle);
    }
}

bool object_manager::is_object_alive(object obj)
{
    std::scoped_lock lock(m_object_mutex);

    return m_objects.is_valid(obj);
}

object_manager::object_state* object_manager::get_object_state(object obj)
{
    if (!m_objects.is_valid(obj))
    {
        return nullptr;
    }

    return &m_objects[obj];
}

component* object_manager::add_component(object handle, std::type_index index)
{
    component_pool_base& pool = *get_component_pool(index);
    component* comp = pool.alloc();
    add_component(handle, comp);
    return comp;
}

void object_manager::add_component(object handle, component* comp)
{
    std::scoped_lock lock(m_object_mutex);

    object_manager::object_state* state = get_object_state(handle);
    if (!state)
    {
        return;
    }

    for (component* existing_comp : state->components)
    {
        if (typeid(*existing_comp) == typeid(*comp))
        {
            db_error(engine, "Attempt to register duplicate component to object. An object can only have a single component of each type.");
            return;
        }
    }

    // Let systems know the component is being added so it can set anything up if it cares.
    {
        std::scoped_lock lock(m_system_mutex);
        for (size_t i = 0; i < m_systems.size(); i++)
        {
            auto& system = m_systems[i];
            system->component_added(handle, comp);
        }
    }

    state->components.push_back(comp);

    update_object_registration(*state);
}

void object_manager::remove_component(object handle, component* component)
{
    std::scoped_lock lock(m_object_mutex);

    /*if (dynamic_cast<meta_component*>(component))
    {
        db_assert_message(false, "Attempted to remove meta_component, this is not supported, this component is internal and should not be touched.");
        return;
    }*/

    if (m_is_system_step_active)
    {
        m_pending_remove_component.push_back({ handle, component });
    }
    else
    {
        commit_remove_component(handle, component, true);
    }
}

void object_manager::remove_component(object handle, std::type_index index)
{
    remove_component(handle, get_component(handle, index));
}

std::vector<component*> object_manager::get_components(object handle)
{
    std::scoped_lock lock(m_object_mutex);

    object_manager::object_state* state = get_object_state(handle);
    if (!state)
    {
        return {};
    }

    return state->components;
}

std::vector<std::type_index> object_manager::get_component_types(object handle)
{
    std::scoped_lock lock(m_object_mutex);

    object_manager::object_state* state = get_object_state(handle);
    if (!state)
    {
        return {};
    }

    std::vector<std::type_index> component_types;
    component_types.reserve(state->components.size());

    for (component* comp : state->components)
    {
        component_types.push_back(typeid(*comp));
    }

    return component_types;
}

component* object_manager::get_component(object handle, std::type_index index)
{
    std::scoped_lock lock(m_object_mutex);

    object_manager::object_state* state = get_object_state(handle);
    if (!state)
    {
        return {};
    }

    for (component* comp : state->components)
    {
        if (index == typeid(*comp))
        {
            return comp;
        }
    }

    return nullptr;
}

std::vector<uint8_t> object_manager::serialize_component(object handle, std::type_index component_type)
{
    std::scoped_lock lock(m_object_mutex);

    std::vector<uint8_t> ret;
    ram_stream output(ret, true);

    component* comp = get_component(handle, component_type);
    if (comp == nullptr)
    {
        db_error(engine, "Attempt to serialize component that doesn't exist on object. Serialized data will be truncated.");
        return {};
    }
    
    reflect_class* comp_class = get_reflect_class(component_type);
    if (comp_class == nullptr)
    {
        db_error(engine, "Attempt to serialize component that doesn't have a reflection class. Serialized data will be truncated.");
        return {};
    }

    std::string comp_class_name = comp_class->get_name();
    stream_serialize(output, comp_class_name);

    for (reflect_field* field : comp_class->get_fields(true))
    {
        std::string field_name = field->get_name();
        stream_serialize(output, field_name);
        stream_serialize_reflect(output, comp, field);
    }

    return ret;
}

void object_manager::deserialize_component(object handle, const std::vector<uint8_t>& data, bool mark_as_edited)
{
    std::scoped_lock lock(m_object_mutex);

    ram_stream output(data);

    std::string comp_class_name;
    stream_serialize(output, comp_class_name);

    reflect_class* comp_class = get_reflect_class(comp_class_name.c_str());
    if (comp_class == nullptr)
    {
        db_error(engine, "Attempt to deserialize component that doesn't have a reflection class '%s'.", comp_class_name.c_str());
        return;
    }

    component* new_component = get_component(handle, comp_class->get_type_index());
    if (new_component == nullptr)
    {
        new_component = add_component(handle, comp_class->get_type_index());
    }

    while (!output.at_end())
    {
        std::string field_name;
        stream_serialize(output, field_name);

        reflect_field* field = comp_class->find_field(field_name.c_str(), true);
        if (field == nullptr)
        {
            db_error(engine, "Attempt to deserialize component that doesn't have a reflection field '%s'.", field_name.c_str());
            return;
        }

        stream_serialize_reflect(output, new_component, field);
    }

    if (mark_as_edited)
    {
        component_edited(handle, new_component, component_modification_source::serialization);
    }
}

std::vector<uint8_t> object_manager::serialize_object(object handle)
{
    std::scoped_lock lock(m_object_mutex);

    std::vector<uint8_t> ret;
    ram_stream output(ret, true);

    std::vector<component*> components = get_components(handle);
    size_t component_count = components.size();

    stream_serialize(output, component_count);
    for (component* comp : components)
    {
        std::vector<uint8_t> comp_payload = serialize_component(handle, typeid(*comp));
        stream_serialize_list(output, comp_payload);
    }

    return ret;
}

void object_manager::deserialize_object(object handle, const std::vector<uint8_t>& data, bool mark_as_edited)
{
    std::scoped_lock lock(m_object_mutex);

    ram_stream output(data);

    // Remove all components this object has on it, we will be deserializing them.
    object_state* obj_state = get_object_state(handle);
    std::vector<component*> existing_components = obj_state->components;
    for (component* comp : existing_components)
    {
        remove_component(handle, comp);
    }

    size_t component_count = 0;
    stream_serialize(output, component_count);

    for (size_t i = 0; i < component_count; i++)
    {
        std::vector<uint8_t> comp_payload;
        stream_serialize_list(output, comp_payload);

        deserialize_component(handle, comp_payload, false);
    }

    // Mark all objects as edited.
    if (mark_as_edited)
    {
        for (component* comp : obj_state->components)
        {
            component_edited(handle, comp, component_modification_source::serialization);
        }
    }
}

void object_manager::step_systems(const frame_time& time)
{
    task_scheduler& scheduler = task_scheduler::get();
    std::vector<task_handle> step_tasks;

    {
        std::scoped_lock lock(m_system_mutex);

        // Update all systems in parallel.
        step_tasks.resize(m_systems.size());

        // Create tasks for stepping each system.
        for (size_t i = 0; i < m_systems.size(); i++)
        {
            auto& system = m_systems[i];

            step_tasks[i] = scheduler.create_task(system->get_name(), task_queue::standard, [this, &time, i]() {
                profile_marker(profile_colors::simulation, "step ecs system: %s", m_systems[i]->get_name());
                m_systems[i]->step(time);
            });
        }

        // Add dependencies between the tasks.
        for (size_t i = 0; i < m_systems.size(); i++)
        {
            auto& main_system = m_systems[i];

            for (system* dependency : main_system->get_dependencies())
            {
                auto iter = std::find_if(m_systems.begin(), m_systems.end(), [dependency](auto& system) {
                    return system.get() == dependency;
                    });
                db_assert(iter != m_systems.end());

                size_t dependency_index = std::distance(m_systems.begin(), iter);
                step_tasks[i].add_dependency(step_tasks[dependency_index]);
            }
        }
    }

    // Dispatch and away for completion.
    m_is_system_step_active = true;

    {
        profile_marker(profile_colors::simulation, "step ecs systems");

        scheduler.dispatch_tasks(step_tasks);
        scheduler.wait_for_tasks(step_tasks, true);
    }

    m_is_system_step_active = false;
}

void object_manager::step(const frame_time& time)
{
    // step all systems.
    step_systems(time);

    // Deferred actions.
    {
        profile_marker(profile_colors::simulation, "executing deferred ecs actions");

        std::scoped_lock lock(m_object_mutex);

        // Run deferred component removal.
        for (auto& obj : m_pending_remove_component)
        {
            commit_remove_component(obj.first, obj.second, true);
        }
        m_pending_remove_component.clear();

        // Run deferred deletions.
        for (object obj : m_pending_destroy)
        {
            destroy_object(obj);
        }
        m_pending_destroy.clear();

        // Run deferred object registration updates.
        for (object obj : m_pending_registration)
        {
            object_state* state = get_object_state(obj);
            if (state == nullptr)
            {
                // Destroy on the frame it was created on.
                continue;
            }
            update_object_registration(*state);
        }
        m_pending_registration.clear();
    }
}

}; // namespace ws
