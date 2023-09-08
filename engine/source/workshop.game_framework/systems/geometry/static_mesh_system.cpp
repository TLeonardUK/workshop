// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/geometry/static_mesh_system.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/geometry/static_mesh_component.h"
#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.renderer/renderer.h"

namespace ws {

static_mesh_system::static_mesh_system(object_manager& manager)
    : system(manager, "static mesh system")
{
    // We want the latest transform to apply to the render object.
    add_predecessor<transform_system>();
}

void static_mesh_system::component_removed(object handle, component* comp)
{
    static_mesh_component* component = dynamic_cast<static_mesh_component*>(comp);
    if (!component)
    {
        return;
    }
    
    render_object_id render_id = component->render_id;
    if (!render_id)
    {
        return;
    }

    m_command_queue.queue_command("destroy_mesh", [this, render_id]() {
        engine& engine = m_manager.get_world().get_engine();
        render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

        render_command_queue.destroy_static_mesh(render_id);
    });
}

void static_mesh_system::component_modified(object handle, component* comp)
{
    static_mesh_component* component = dynamic_cast<static_mesh_component*>(comp);
    if (!component)
    {
        return;
    }

    component->is_dirty = true;
}

void static_mesh_system::set_model(object handle, asset_ptr<model> model)
{
    m_command_queue.queue_command("set_model", [this, handle, model]() {
        static_mesh_component* comp = m_manager.get_component<static_mesh_component>(handle);
        if (comp)
        {
            engine& engine = m_manager.get_world().get_engine();
            render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

            comp->model = model;
            render_command_queue.set_static_mesh_model(comp->render_id, model);
        }
    });
}

void static_mesh_system::set_render_gpu_flags(object handle, render_gpu_flags flags)
{
    m_command_queue.queue_command("set_render_gpu_flags", [this, handle, flags]() {
        static_mesh_component* comp = m_manager.get_component<static_mesh_component>(handle);
        if (comp)
        {
            engine& engine = m_manager.get_world().get_engine();
            render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

            comp->render_gpu_flags = flags;
            render_command_queue.set_object_gpu_flags(comp->render_id, flags);
        }
    });
}

void static_mesh_system::step(const frame_time& time)
{
    engine& engine = m_manager.get_world().get_engine();
    renderer& render = engine.get_renderer();
    render_command_queue& render_command_queue = render.get_command_queue();

    component_filter<static_mesh_component, const transform_component> filter(m_manager);
    for (size_t i = 0; i < filter.size(); i++)
    {
        object obj = filter.get_object(i);

        const transform_component* transform = filter.get_component<transform_component>(i);
        static_mesh_component* light = filter.get_component<static_mesh_component>(i);

        // Create render object if it doesn't exist yet.
        if (light->render_id == null_render_object)
        {
            light->render_id = render_command_queue.create_static_mesh("Static Mesh");
            light->is_dirty = true;
        }

        // Apply changes if dirty.
        if (light->is_dirty)
        {
            render_command_queue.set_static_mesh_model(light->render_id, light->model);
            render_command_queue.set_object_gpu_flags(light->render_id, light->render_gpu_flags);
            light->is_dirty = false;
        }

        // Apply object transform if its changed.
        if (transform->generation != light->last_transform_generation)
        {
            light->last_transform_generation = transform->generation;
            render_command_queue.set_object_transform(light->render_id, transform->world_location, transform->world_rotation, transform->world_scale);
        }
    }

    // Execute all commands after creating the render objects.
    flush_command_queue();
}

}; // namespace ws
