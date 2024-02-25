// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/physics/physics_system.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.engine/ecs/meta_component.h"
#include "workshop.game_framework/components/camera/camera_component.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/physics/physics_component.h"
#include "workshop.game_framework/components/physics/physics_capsule_component.h"
#include "workshop.game_framework/components/physics/physics_box_component.h"
#include "workshop.game_framework/components/physics/physics_sphere_component.h"
#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.renderer/renderer.h"

namespace ws {

physics_system::physics_system(object_manager& manager)
    : system(manager, "physics system")
{
    set_flags(system_flags::run_in_editor);

    // We want to syncronize our physics positions before running the transform update.
    add_successor<transform_system>();
}

void physics_system::component_removed(object handle, component* comp)
{
    physics_component* component = dynamic_cast<physics_component*>(comp);
    if (!component)
    {
        return;
    }
    
    component->physics_body = nullptr;
}

void physics_system::component_modified(object handle, component* comp, component_modification_source source)
{
    physics_component* component = dynamic_cast<physics_component*>(comp);
    if (!component)
    {
        return;
    }

    component->is_dirty = true;
}

void physics_system::step(const frame_time& time)
{
    engine& engine = m_manager.get_world().get_engine();
    pi_world& physics_world = m_manager.get_world().get_physics_world();
    transform_system* trans_system = m_manager.get_system<transform_system>();

    component_filter<physics_component, const transform_component, const meta_component> filter(m_manager);
    for (size_t i = 0; i < filter.size(); i++)
    {
        object obj = filter.get_object(i);

        physics_component* physics = filter.get_component<physics_component>(i);
        meta_component* meta = filter.get_component<meta_component>(i);
        const transform_component* transform = filter.get_component<transform_component>(i);

        // If object scale has changed, mark physics as dirty.
        if (transform->world_scale != physics->last_world_scale)
        {
            physics->is_dirty = true;
            physics->last_world_scale = transform->world_scale;
        }

        // Apply changes if dirty.
        if (physics->is_dirty || physics->physics_body == nullptr)
        {
            pi_body::create_params create_params;
            create_params.collision_type = string_hash(physics->collision_type);
            create_params.dynamic = physics->dynamic;

            bool has_shape = false;

            if (physics_box_component* shape = m_manager.get_component<physics_box_component>(obj))
            {
                create_params.shape.shape = pi_shape::type::box;
                create_params.shape.extents = shape->extents * transform->world_scale;
                has_shape = true;
            }
            else if (physics_capsule_component* shape = m_manager.get_component<physics_capsule_component>(obj))
            {
                create_params.shape.shape = pi_shape::type::capsule;
                create_params.shape.height = shape->height * transform->world_scale.y;
                create_params.shape.radius = shape->radius * math::max(transform->world_scale.x, transform->world_scale.z);
                has_shape = true;
            }
            else if (physics_sphere_component* shape = m_manager.get_component<physics_sphere_component>(obj))
            {
                create_params.shape.shape = pi_shape::type::sphere;
                create_params.shape.radius = shape->radius * transform->world_scale.max_component();
                has_shape = true;
            }

            if (has_shape)
            {
                physics->physics_body = physics_world.create_body(create_params, meta->name.c_str());
                physics_world.add_body(*physics->physics_body);
            }
            else
            {
                physics->physics_body = nullptr;
            }
        }

        vector3 location = transform->world_location;
        quat    rotation = transform->world_rotation;

        // Skip syncing body if we don't have one yet.
        if (physics->physics_body)
        {
            // If transform has changed since last time we synced physics, move the physics body.
            if (transform->world_location != physics->last_world_location || 
                transform->world_rotation != physics->last_world_rotation ||
                physics->is_dirty)
            {
                physics->physics_body->set_transform(transform->world_location, transform->world_rotation);
            }
            // Grab the physics transform, and update the object transform if they differ.
            else
            {
                physics->physics_body->get_transform(location, rotation);

                if (transform->world_location != location ||
                    transform->world_rotation != rotation)
                {
                    trans_system->set_world_transform(obj, location, rotation, transform->world_scale);
                }
            }
        }

        // Store current transform.
        physics->last_world_location = location;
        physics->last_world_rotation = rotation;

        physics->is_dirty = false;
    }

    // Execute all commands after creating the render objects.
    flush_command_queue();

    // Draw debug for any render views that are requesting it.
    draw_debug();
}

void physics_system::draw_debug()
{
    // This is all hot garbage, its not properly culled. 
    // We should probably just create render objects and attach them to the physics bodies and
    // turn their visibility on/off.

    engine& engine = m_manager.get_world().get_engine();
    renderer& render = engine.get_renderer();
    render_command_queue& queue = render.get_command_queue();

    component_filter<camera_component, const transform_component> filter(m_manager);
    for (size_t i = 0; i < filter.size(); i++)
    {
        object obj = filter.get_object(i);

        const transform_component* transform = filter.get_component<transform_component>(i);
        camera_component* camera = filter.get_component<camera_component>(i);

        if ((camera->view_flags & render_view_flags::draw_collision) != render_view_flags::none)
        {
            // Draw boxes
            {
                component_filter<const transform_component, const physics_box_component> filter(m_manager);
                for (size_t i = 0; i < filter.size(); i++)
                {
                    object obj = filter.get_object(i);
                    const transform_component* transform = filter.get_component<transform_component>(i);
                    const physics_box_component* shape = filter.get_component<physics_box_component>(i);

                    vector3 half_extents = shape->extents * 0.5f;

                    obb bounds(aabb(-half_extents, half_extents), transform->local_to_world);
                    queue.draw_obb(bounds, color::red, camera->view_id);
                }
            }

            // Draw spheres
            {
                component_filter<const transform_component, const physics_sphere_component> filter(m_manager);
                for (size_t i = 0; i < filter.size(); i++)
                {
                    object obj = filter.get_object(i);
                    const transform_component* transform = filter.get_component<transform_component>(i);
                    const physics_sphere_component* shape = filter.get_component<physics_sphere_component>(i);

                    queue.draw_sphere(sphere(transform->world_location, shape->radius * transform->world_scale.max_component()), color::red, camera->view_id);
                }
            }

            // Draw Capsules
            {
                component_filter<const transform_component, const physics_capsule_component> filter(m_manager);
                for (size_t i = 0; i < filter.size(); i++)
                {
                    object obj = filter.get_object(i);
                    const transform_component* transform = filter.get_component<transform_component>(i);
                    const physics_capsule_component* shape = filter.get_component<physics_capsule_component>(i);

                    cylinder bounds(transform->world_location, transform->world_rotation, shape->radius, shape->height);
                    queue.draw_capsule(bounds, color::red, camera->view_id);
                }
            }
        }
    }
}

}; // namespace ws
