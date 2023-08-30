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

#pragma optimize("", off)

bounds_system::bounds_system(object_manager& manager)
    : system(manager, "bounds system")
{
    add_predecessor<transform_system>();    
}

void bounds_system::step(const frame_time& time)
{
    engine& engine = m_manager.get_world().get_engine();
    renderer& render = engine.get_renderer();
    render_command_queue& render_command_queue = render.get_command_queue();

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
            mesh->model.get_version() != bounds->last_model_version)
        {
            bounds->local_bounds = obb(mesh->model->geometry->bounds, matrix4::identity);
            bounds->world_bounds = obb(mesh->model->geometry->bounds, transform->local_to_world);

            bounds->last_transform_generation = transform->generation;
            bounds->last_model_version = mesh->model.get_version();
        }
    }

    // Execute all commands after creating the render objects.
    flush_command_queue();
}

obb bounds_system::get_combined_bounds(const std::vector<object>& objects, float default_bounds, vector3& pivot_point, bool consume_pivot_point)
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
        if (bounds)
        {
            world_bounds = bounds->world_bounds;
        }
        else
        {
            world_bounds = obb(aabb(
                    vector3(-default_bounds, -default_bounds, -default_bounds),
                    vector3(default_bounds, default_bounds, default_bounds)
                ),
                transform->local_to_world
            );
        }

        // TODO: We shouldn't have to use AABB for everything, but it makes some logic a lot simpler ...
        world_bounds = obb(bounds->world_bounds.get_aligned_bounds(), matrix4::identity);

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

    // Center the bounds so we get a gizmo in the center not at 0,0,0.
    // Note: Center on the bounds of a single object rather than the center of the combined OBB. Gives better
    //       results when doing things like using scaling gizmos.
//    if (objects.size() > 1)
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

}; // namespace ws
