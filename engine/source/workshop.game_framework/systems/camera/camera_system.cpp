// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/camera/camera_system.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/camera/camera_component.h"
#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.renderer/renderer.h"

namespace ws {

camera_system::camera_system(object_manager& manager)
    : system(manager, "camera system")
{
    // We want the latest transform to apply to the render view.
    add_predecessor<transform_system>();
}

void camera_system::set_projection(object handle, float fov, float aspect_ratio, float min_depth, float max_depth)
{
    m_command_queue.queue_command("set_projection", [this, handle, fov, aspect_ratio, min_depth, max_depth]() {
        camera_component* component = m_manager.get_component<camera_component>(handle);
        if (component)
        {
            component->fov = fov;
            component->aspect_ratio = aspect_ratio;
            component->min_depth = min_depth;
            component->max_depth = max_depth;

            if (component->view_id != null_render_object)
            {
                engine& engine = m_manager.get_world().get_engine();
                render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

                render_command_queue.set_view_projection(component->view_id, component->fov, component->aspect_ratio, component->min_depth, component->max_depth);
            }
        }
    });
}

void camera_system::component_removed(object handle, component* comp)
{
    camera_component* component = dynamic_cast<camera_component*>(comp);
    if (!component)
    {
        return;
    }
    
    render_object_id view_id = component->view_id;
    if (!view_id)
    {
        return;
    }

    m_command_queue.queue_command("destroy_render_view", [this, view_id]() {
        engine& engine = m_manager.get_world().get_engine();
        render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

        render_command_queue.destroy_view(view_id);
    });
}

void camera_system::component_modified(object handle, component* comp)
{
    camera_component* component = dynamic_cast<camera_component*>(comp);
    if (!component)
    {
        return;
    }

    component->is_dirty = true;
}

void camera_system::step(const frame_time& time)
{
    engine& engine = m_manager.get_world().get_engine();
    renderer& render = engine.get_renderer();
    render_command_queue& render_command_queue = render.get_command_queue();
    vector2 screen_size = vector2((float)render.get_display_width(), (float)render.get_display_height());

    // Execute all commands.
    flush_command_queue();

    component_filter<camera_component, const transform_component> filter(m_manager);
    for (size_t i = 0; i < filter.size(); i++)
    {
        object obj = filter.get_object(i);

        const transform_component* transform = filter.get_component<transform_component>(i);
        camera_component* camera = filter.get_component<camera_component>(i);
        bool update_matrices = false;

        // Create view if it doesn't exist yet.
        if (camera->view_id == null_render_object)
        {
            camera->view_id = render_command_queue.create_view("Camera");
            camera->is_dirty = true;
        }

        // Apply settings if component is dirty.
        if (camera->is_dirty)
        {
            render_command_queue.set_view_viewport(camera->view_id, recti(0, 0, static_cast<int>(screen_size.x), static_cast<int>(screen_size.y)));
            render_command_queue.set_view_projection(camera->view_id, camera->fov, camera->aspect_ratio, camera->min_depth, camera->max_depth);
            render_command_queue.set_object_transform(camera->view_id, transform->world_location, transform->world_rotation, transform->world_scale);
            update_matrices = true;

            camera->is_dirty = false;
        }

        // Apply object transform if its changed.
        if (transform->generation != camera->last_transform_generation)
        {
            camera->last_transform_generation = transform->generation;
            render_command_queue.set_object_transform(camera->view_id, transform->world_location, transform->world_rotation, transform->world_scale);

            update_matrices = true;
        }

        if (update_matrices)
        {
            camera->projection_matrix = matrix4::perspective(
                math::radians(camera->fov),
                camera->aspect_ratio,
                camera->min_depth,
                camera->max_depth);

            camera->view_matrix = matrix4::look_at(
                transform->world_location,
                transform->world_location + (vector3::forward * transform->world_rotation),
                vector3::up);
        }
    }
}

}; // namespace ws
