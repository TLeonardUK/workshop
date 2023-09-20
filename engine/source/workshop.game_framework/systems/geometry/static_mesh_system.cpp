// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/geometry/static_mesh_system.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.engine/ecs/meta_component.h"
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

void static_mesh_system::component_modified(object handle, component* comp, component_modification_source source)
{
    static_mesh_component* component = dynamic_cast<static_mesh_component*>(comp);
    if (!component)
    {
        return;
    }

    // If user modified the component, mark the materials vector as needing an update.
    if (source == component_modification_source::user)
    {
        if (component->model != component->last_model)
        {
            component->materials_array_needs_update = true;
        }
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
            comp->materials.clear();
            
            render_command_queue.set_static_mesh_materials(comp->render_id, { });
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

    component_filter<static_mesh_component, const transform_component, const meta_component> filter(m_manager);
    for (size_t i = 0; i < filter.size(); i++)
    {
        object obj = filter.get_object(i);

        const meta_component* meta = filter.get_component<meta_component>(i);
        const transform_component* transform = filter.get_component<transform_component>(i);
        static_mesh_component* light = filter.get_component<static_mesh_component>(i);

        // Create render object if it doesn't exist yet.
        if (light->render_id == null_render_object)
        {
            light->render_id = render_command_queue.create_static_mesh("Static Mesh");
            light->is_dirty = true;
        }

        // If materials list is empty fill it out with defaults of the model so the user can modify them.
        if ((light->materials.empty() || light->materials_array_needs_update) && light->model.is_loaded())
        {
            light->materials.clear();
            light->materials.reserve(light->model->materials.size());
            
            for (auto& mat_info : light->model->materials)
            {
                light->materials.push_back(mat_info.material);
            }
            
            light->is_dirty = true;
            light->materials_array_needs_update = false;
        }

        // Apply changes if dirty.
        if (light->is_dirty)
        {
            render_command_queue.set_static_mesh_materials(light->render_id, light->materials);
            render_command_queue.set_static_mesh_model(light->render_id, light->model);
            render_command_queue.set_object_gpu_flags(light->render_id, light->render_gpu_flags);
            render_command_queue.set_object_draw_flags(light->render_id, light->render_draw_flags);
            light->is_dirty = false;
        }
    
        // Apply object transform if its changed.
        if (transform->generation != light->last_transform_generation)
        {
            light->last_transform_generation = transform->generation;
            render_command_queue.set_object_transform(light->render_id, transform->world_location, transform->world_rotation, transform->world_scale);
        }

        // Mark the render primitives as selected for the renderer.
        bool should_be_selected = (meta->flags & object_flags::selected) != object_flags::none;
        bool is_selected = (light->render_gpu_flags & render_gpu_flags::selected) != render_gpu_flags::none;

        if (should_be_selected != is_selected)
        {
            if (should_be_selected)
            {
                light->render_gpu_flags = light->render_gpu_flags | render_gpu_flags::selected;
            }
            else
            {
                light->render_gpu_flags = light->render_gpu_flags & ~render_gpu_flags::selected;
            }

            render_command_queue.set_object_gpu_flags(light->render_id, light->render_gpu_flags);
        }

        light->last_model = light->model;
    }

    // Execute all commands after creating the render objects.
    flush_command_queue();
}

}; // namespace ws
