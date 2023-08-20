// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/ecs/object_manager.h"
#include "workshop.core/async/task_scheduler.h"
#include "workshop.core/perf/profile.h"

namespace ws {

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

    object_state state;
    state.handle = m_next_object_handle++;

    m_objects.emplace(state.handle, state);

    if (m_is_system_step_active)
    {
        m_pending_registration.push_back(state.handle);
    }
    else
    {
        update_object_registration(state);
    }

    return state.handle;
}

void object_manager::update_object_registration(object_state& obj)
{
    // TODO: register with all filters.
}

void object_manager::destroy_object(object obj)
{
    std::scoped_lock lock(m_object_mutex);

    auto iter = m_objects.find(obj);
    if (iter == m_objects.end())
    {
        return;
    }

    if (m_is_system_step_active)
    {
        m_pending_destroy.push_back(obj);
    }
    else
    {
        commit_destroy_object(obj);
    }
}

void object_manager::commit_destroy_object(object obj)
{
    auto iter = m_objects.find(obj);
    if (iter == m_objects.end())
    {
        return;
    }

    // TODO: erase all components.

    m_objects.erase(iter);
}

bool object_manager::is_object_alive(object obj)
{
    std::scoped_lock lock(m_object_mutex);

    return m_objects.find(obj) != m_objects.end();
}

object_manager::object_state* object_manager::get_object_state(object obj)
{
    auto iter = m_objects.find(obj);
    if (iter == m_objects.end())
    {
        return nullptr;
    }

    return &iter->second;
}

void object_manager::remove_component(object handle, component* component)
{
    // TODO
}

std::vector<component*> object_manager::get_components(object handle)
{
    // TODO
    return {};
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
