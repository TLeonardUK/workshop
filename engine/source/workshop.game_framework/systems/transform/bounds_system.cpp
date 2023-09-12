// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/transform/bounds_system.h"
#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.engine/engine/world.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.renderer/assets/model/model.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/transform/bounds_component.h"
#include "workshop.game_framework/components/geometry/static_mesh_component.h"
#include "workshop.core/async/task_scheduler.h"
#include "workshop.core/async/async.h"
#include "workshop.core/perf/profile.h"

namespace ws {

bounds_system::bounds_system(object_manager& manager)
    : system(manager, "bounds system")
    , m_oct_tree(k_octtree_extents, k_octtree_max_depth)
{
    add_predecessor<transform_system>();    
}

void bounds_system::component_removed(object handle, component* comp)
{
    bounds_component* bounds = dynamic_cast<bounds_component*>(comp);
    if (bounds && bounds->octree_token.is_valid())
    {
        m_oct_tree.remove(bounds->octree_token);
        bounds->octree_token = {};
    }
}

void bounds_system::step(const frame_time& time)
{
    engine& engine = m_manager.get_world().get_engine();
    renderer& render = engine.get_renderer();
    render_command_queue& render_command_queue = render.get_command_queue();

    std::vector<std::pair<object, bounds_component*>> modified_bounds;

    // Calculate bounds for any components with static meshes.
    component_filter<transform_component, bounds_component, static_mesh_component> filter(m_manager);
    for (size_t i = 0; i < filter.size(); i++)
    {
        object obj = filter.get_object(i);

        const transform_component* transform = filter.get_component<transform_component>(i);
        static_mesh_component* mesh = filter.get_component<static_mesh_component>(i);
        bounds_component* bounds = filter.get_component<bounds_component>(i);
        
        if (!mesh->model.is_loaded())
        {
            continue;
        }

        if (transform->generation != bounds->last_transform_generation ||
            mesh->model.get_version() != bounds->last_model_version ||
            mesh->model.get_hash() != bounds->last_model_hash)
        {
            bounds->local_bounds = obb(mesh->model->geometry->bounds, matrix4::identity);
            bounds->world_bounds = obb(mesh->model->geometry->bounds, transform->local_to_world);

            bounds->last_transform_generation = transform->generation;
            bounds->last_model_version = mesh->model.get_version();
            bounds->is_valid = true;
            bounds->has_bounds_source = true;

            modified_bounds.push_back({ obj, bounds });
        }
    }

    // Apply a default to any components that don't have any components we can calcualte bounds from.
    component_filter<transform_component, bounds_component> all_filter(m_manager);
    for (size_t i = 0; i < all_filter.size(); i++)
    {
        object obj = all_filter.get_object(i);

        const transform_component* transform = all_filter.get_component<transform_component>(i);
        bounds_component* bounds = all_filter.get_component<bounds_component>(i);

        if (!bounds->has_bounds_source && transform->generation != bounds->last_transform_generation)
        {
            aabb unit_bounds(vector3(-0.5, -0.5, -0.5), vector3(0.5f, 0.5f, 0.5f));

            bounds->local_bounds = obb(unit_bounds, matrix4::identity);
            bounds->world_bounds = obb(unit_bounds, transform->local_to_world);
            bounds->is_valid = true;

            modified_bounds.push_back({ obj, bounds });
        }
    }

    // All components that have had their bounds changed need to update their octtree registration.
    for (auto& [obj, bounds] : modified_bounds)
    {
        if (bounds->octree_token.is_valid())
        {
            bounds->octree_token = m_oct_tree.modify(bounds->octree_token, bounds->world_bounds.get_aligned_bounds(), obj);
        }
        else
        {
            bounds->octree_token = m_oct_tree.insert(bounds->world_bounds.get_aligned_bounds(), obj);
        }
    }

    // Execute all commands after creating the render objects.
    flush_command_queue();
}

obb bounds_system::get_combined_bounds(const std::vector<object>& objects, vector3& pivot_point, bool consume_pivot_point)
{    
    obb object_bounds;
    bool first_object = true;
    vector3 first_object_center = vector3::zero;

    for (object obj : objects)
    {
        transform_component* transform = m_manager.get_component<transform_component>(obj);
        if (!transform)
        {
            continue;
        }

        obb world_bounds;
        vector3 world_origin = transform->local_to_world.transform_location(vector3::zero);

        bounds_component* bounds = m_manager.get_component<bounds_component>(obj);
        if (bounds && bounds->is_valid)
        {
            world_bounds = bounds->world_bounds;
        }
        else
        {
            world_bounds = obb(aabb(
                    vector3(-k_default_bounds, -k_default_bounds, -k_default_bounds),
                    vector3(k_default_bounds, k_default_bounds, k_default_bounds)
                ),
                transform->local_to_world
            );
        }

        // In theory we can concatenate OBB's, but for consistency we are just always using AABB for now.
        world_bounds = obb(world_bounds.get_aligned_bounds(), matrix4::identity);

        if (!first_object)
        {
            object_bounds = object_bounds.combine(world_bounds);
        }
        else
        {
            first_object_center = world_origin;
            object_bounds = world_bounds; 
            first_object = false;
        }
    }

    // Offset bounds by pivot, nicer experience than the origin being 0,0,0.
    {
        vector3 center = first_object_center;
        if (consume_pivot_point)
        {
            center = pivot_point;
        }
        else
        {
            pivot_point = center;
        }

        aabb bounds(object_bounds.bounds.min - center, object_bounds.bounds.max - center);
        object_bounds = obb(bounds, matrix4::translate(center));
    }

    return object_bounds;
}

std::vector<object> bounds_system::intersects(const ray& target_ray)
{
    std::vector<object> result;

    oct_tree<object>::intersect_result intersections = m_oct_tree.intersect(target_ray, false, false);
    for (auto& entry : intersections.entries)
    {
        result.push_back(entry->value);
    }

    return result;
}

}; // namespace ws
