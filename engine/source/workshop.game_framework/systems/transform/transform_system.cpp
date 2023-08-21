// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.core/async/task_scheduler.h"
#include "workshop.core/async/async.h"
#include "workshop.core/perf/profile.h"

namespace ws {

transform_system::transform_system(object_manager& manager)
    : system(manager, "transform system")
{
}

void transform_system::set_local_transform(object handle, const vector3& location, const quat& rotation, const vector3& scale)
{
    m_command_queue.queue_command("set_local_transform", [this, handle, location, rotation, scale]() {
        transform_component* component = m_manager.get_component<transform_component>(handle);
        if (component)
        {
            component->local_location = location;
            component->local_rotation = rotation;
            component->local_scale = scale;
            component->is_dirty = true;
        }
    });
}

void transform_system::set_parent(object handle, object parent)
{
    m_command_queue.queue_command("set_parent", [this, handle, parent]() {
        transform_component* component = m_manager.get_component<transform_component>(handle);
        if (component)
        {
            component->parent = parent;
            component->is_dirty = true;
        }
    });
}

void transform_system::update_transform(transform_component* transform, transform_component* parent_transform)
{
    transform->local_to_world = matrix4::scale(transform->local_scale) *
                                matrix4::rotation(transform->local_rotation) *
                                matrix4::translate(transform->local_location);

    if (parent_transform != nullptr)
    {
        transform->local_to_world = transform->local_to_world * parent_transform->local_to_world;
    }

    transform->world_to_local = transform->local_to_world.inverse();
    transform->is_dirty = false;
    transform->generation++;
    
    if (transform->children.size() < k_async_update_transform_threshold)
    {
        for (size_t i = 0; i < transform->children.size(); i++)
        {
            transform_component* child = transform->children[i].get(&m_manager);
            update_transform(child, transform);
        }
    }
    else
    { 
        parallel_for("update child transforms", task_queue::standard, transform->children.size(), [this, &transform](size_t index) {
            transform_component* child = transform->children[index].get(&m_manager);
            update_transform(child, transform);
        });
    }
}

void transform_system::step(const frame_time& time)
{
    component_filter<transform_component> filter(m_manager);

    // Execute all commands.
    flush_command_queue();

    // Find root of dirty trees.
    task_scheduler& scheduler = task_scheduler::get();
    size_t split_factor = scheduler.get_worker_count(task_queue::standard) * 4;
    size_t chunk_size = static_cast<size_t>(math::ceil(filter.size() / static_cast<float>(split_factor)));

    m_dirty_roots.resize(split_factor);
    m_dirty_roots_list.resize(split_factor);

    // Combine lists together.
    {
        profile_marker(profile_colors::system, "find dirty roots");
        
        parallel_for("find dirty roots", task_queue::standard, split_factor, [this, chunk_size, &filter](size_t index) {
            
            size_t start_index = (index * chunk_size);
            size_t end_index = std::min(filter.size(), start_index + chunk_size);

            m_dirty_roots[index].clear();
            m_dirty_roots_list[index].clear();

            for (size_t i = start_index; i < end_index; i++)
            {
                object obj = filter.get_object(i);
                transform_component* transform = filter.get_component<transform_component>(i);

                if (transform->is_dirty)
                {
                    transform_component* last_dirty = transform;

                    while (transform != nullptr)
                    {
                        if (transform->is_dirty)
                        {
                            last_dirty = transform;
                        }
                        transform = transform->parent.get(&m_manager);
                    }

                    if (m_dirty_roots[index].emplace(last_dirty).second)
                    {
                        m_dirty_roots_list[index].push_back(last_dirty);
                    }
                }
            }

        });
    }

    // Combine the lists from each split.
    {
        profile_marker(profile_colors::system, "combine dirty roots list");

        for (size_t i = 1; i < split_factor; i++)
        {
            std::vector<transform_component*>& split_list = m_dirty_roots_list[i];

            for (transform_component* component : split_list)
            {
                if (m_dirty_roots[0].emplace(component).second)
                {
                    m_dirty_roots_list[0].push_back(component);
                }
            }
        }
    }

    // Update all dirty transforms.
    {
        profile_marker(profile_colors::system, "update dirty roots");
        
        parallel_for("update dirty roots", task_queue::standard, m_dirty_roots_list[0].size(), [this](size_t index) {
            profile_marker(profile_colors::system, "update dirty roots task");
            transform_component* component = m_dirty_roots_list[0][index];
            transform_component* parent = m_dirty_roots_list[0][index]->parent.get(&m_manager);
            update_transform(component, parent);
        });
    }
}

}; // namespace ws
