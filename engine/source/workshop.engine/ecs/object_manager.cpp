// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/ecs/object_manager.h"
#include "workshop.engine/ecs/component_filter_archetype.h"
#include "workshop.core/async/task_scheduler.h"
#include "workshop.core/perf/profile.h"

namespace ws {

object_manager::object_manager()
    : m_objects(k_max_objects)
{
    // Always allocate the first index so we can assume 0=null.
    size_t index = m_objects.insert({});
    db_assert(index == 0);
}

system* object_manager::get_system(const std::type_index& type_info)
{
    std::scoped_lock lock(m_system_mutex);

    auto iter = std::find_if(m_systems.begin(), m_systems.end(), [&type_info](const std::unique_ptr<system>& sys) {
        std::type_index sys_index = typeid(sys.get());
        return sys_index == type_info;
    });

    if (iter != m_systems.end())
    {
        return iter->get();
    }

    return nullptr;
}

object object_manager::create_object()
{
    std::scoped_lock lock(m_object_mutex);

    size_t index = m_objects.insert({});
    object_state& state = m_objects[index];
    state.handle = index;

    update_object_registration(state);
    
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

component_filter_archetype* object_manager::get_filter_archetype(const std::vector<std::type_index>& unsorted_component_types)
{
    std::scoped_lock lock(m_object_mutex);

    // Sort into a deterministic order so filters that just have an order difference
    // don't create an entirely new archetype.
    std::vector<std::type_index> component_types = unsorted_component_types;
    std::sort(component_types.begin(), component_types.end());

    component_types_key key;
    key.component_types = component_types;

    auto iter = m_component_filter_archetype.find(key);
    if (iter != m_component_filter_archetype.end())
    {
        return iter->second.get();
    }

    std::unique_ptr<component_filter_archetype> archetype = std::make_unique<component_filter_archetype>(*this, component_types);
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

void object_manager::commit_destroy_object(object handle)
{
    object_manager::object_state* state = get_object_state(handle);
    if (!state)
    {
        return;
    }

    for (auto& pair : m_component_filter_archetype)
    {
        pair.second->remove_object(handle);
    }

    std::vector<component*> to_remove = state->components;
    for (component* comp : to_remove)
    {
        commit_remove_component(handle, comp);
    }
    
    m_objects.remove(handle);
}

void object_manager::commit_remove_component(object handle, component* comp)
{
    object_manager::object_state* state = get_object_state(handle);
    if (!state)
    {
        return;
    }

    auto iter = std::find(state->components.begin(), state->components.end(), comp);
    if (iter != state->components.end())
    {
        component_pool_base* base = get_component_pool(typeid(*comp));
        base->free(comp);

        state->components.erase(iter);
    }

    update_object_registration(*state);
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

    state->components.push_back(comp);

    update_object_registration(*state);
}

void object_manager::remove_component(object handle, component* component)
{
    std::scoped_lock lock(m_object_mutex);

    if (m_is_system_step_active)
    {
        m_pending_remove_component.push_back({ handle, component });
    }
    else
    {
        commit_remove_component(handle, component);
    }
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

std::vector<std::type_index> object_manager::get_component_tyes(object handle)
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

void object_manager::step_systems(const frame_time& time)
{
    std::scoped_lock lock(m_system_mutex);

    // Update all systems in parallel.
    task_scheduler& scheduler = task_scheduler::get();
    std::vector<task_handle> step_tasks;
    step_tasks.resize(m_systems.size());

    // Create tasks for stepping each system.
    for (size_t i = 0; i < m_systems.size(); i++)
    {
        auto& system = m_systems[i];

        step_tasks[i] = scheduler.create_task(system->get_name(), task_queue::standard, [this, &time, i]() {
            profile_marker(profile_colors::simulation, "step system: %s", m_systems[i]->get_name());
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

    // Dispatch and away for completion.
    m_is_system_step_active = true;

    scheduler.dispatch_tasks(step_tasks);
    scheduler.wait_for_tasks(step_tasks, true);

    m_is_system_step_active = false;
}

void object_manager::step(const frame_time& time)
{
    // step all systems.
    step_systems(time);

    // Deferred actions.
    {
        std::scoped_lock lock(m_object_mutex);

        // Run deferred component removal.
        for (auto& obj : m_pending_remove_component)
        {
            commit_remove_component(obj.first, obj.second);
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
