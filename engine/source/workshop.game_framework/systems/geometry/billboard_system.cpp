// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/geometry/billboard_system.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.engine/ecs/meta_component.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/geometry/billboard_component.h"
#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.renderer/renderer.h"

namespace ws {

billboard_system::billboard_system(object_manager& manager)
    : system(manager, "billboard system")
{
    set_flags(system_flags::run_in_editor);

    // We want the latest transform to apply to the render object.
    add_predecessor<transform_system>();
}

void billboard_system::component_removed(object handle, component* comp)
{
    billboard_component* component = dynamic_cast<billboard_component*>(comp);
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

void billboard_system::component_modified(object handle, component* comp, component_modification_source source)
{
    billboard_component* component = dynamic_cast<billboard_component*>(comp);
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

void billboard_system::set_model(object handle, asset_ptr<model> model)
{
    m_command_queue.queue_command("set_model", [this, handle, model]() {
        billboard_component* comp = m_manager.get_component<billboard_component>(handle);
        if (comp)
        {
            engine& engine = m_manager.get_world().get_engine();
            renderer& render = engine.get_renderer();
            render_command_queue& render_command_queue = render.get_command_queue();

            comp->model = model;
            comp->materials.clear();

            auto default_model = render.get_debug_model(debug_model::plane);

            render_command_queue.set_static_mesh_materials(comp->render_id, { });
            render_command_queue.set_static_mesh_model(comp->render_id, comp->model.is_valid() ? model : default_model);
        }
    });
}

void billboard_system::set_render_gpu_flags(object handle, render_gpu_flags flags)
{
    m_command_queue.queue_command("set_render_gpu_flags", [this, handle, flags]() {
        billboard_component* comp = m_manager.get_component<billboard_component>(handle);
        if (comp)
        {
            engine& engine = m_manager.get_world().get_engine();
            render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

            comp->render_gpu_flags = flags;
            render_command_queue.set_object_gpu_flags(comp->render_id, flags);
        }
    });
}

void billboard_system::step(const frame_time& time)
{
    engine& engine = m_manager.get_world().get_engine();
    renderer& render = engine.get_renderer();
    render_command_queue& render_command_queue = render.get_command_queue();
    transform_system* transform_sys = m_manager.get_system<transform_system>();

    object primary_camera = m_manager.get_world().get_primary_camera();

    component_filter<billboard_component, const transform_component, const meta_component> filter(m_manager);
    for (size_t i = 0; i < filter.size(); i++)
    {
        object obj = filter.get_object(i);

        const meta_component* meta = filter.get_component<meta_component>(i);
        const transform_component* transform = filter.get_component<transform_component>(i);
        billboard_component* light = filter.get_component<billboard_component>(i);

        // Create render object if it doesn't exist yet.
        if (light->render_id == null_render_object)
        {
            light->render_id = render_command_queue.create_static_mesh("Billboard");
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
            if (!light->model.is_valid())
            {
                light->model = render.get_debug_model(debug_model::plane);
            }

            render_command_queue.set_static_mesh_materials(light->render_id, light->materials);
            render_command_queue.set_static_mesh_model(light->render_id, light->model);
            render_command_queue.set_object_gpu_flags(light->render_id, light->render_gpu_flags);
            render_command_queue.set_object_draw_flags(light->render_id, light->render_draw_flags);
            light->is_dirty = false;
        }

        // Update billboard direction.
        // TODO: we probably want to do the rotation here in the render system so objects always
        // face the correct direction for all views.
        if (primary_camera)
        {
            transform_component* camera_transform = m_manager.get_component<transform_component>(primary_camera);

            vector3 camera_up = vector3::up * camera_transform->world_rotation;
            matrix4 look_at_matrix = matrix4::look_at(transform->world_location, camera_transform->world_location, camera_up).inverse();
            quat rotation = look_at_matrix.extract_rotation();

            light->transform = 
                matrix4::scale(vector3(light->size, light->size, light->size)) *
                matrix4::rotation(transform->world_rotation.inverse() * rotation);

            render_command_queue.set_object_transform(light->render_id, transform->world_location, rotation, transform->world_scale * vector3(light->size, light->size, light->size));
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
