// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/lighting/light_probe_grid_system.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/lighting/light_probe_grid_component.h"
#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.renderer/renderer.h"

namespace ws {

light_probe_grid_system::light_probe_grid_system(object_manager& manager)
    : system(manager, "light probe grid system")
{
    set_flags(system_flags::run_in_editor);

    // We want the latest transform to apply to the render object.
    add_predecessor<transform_system>();
}

void light_probe_grid_system::set_grid_density(object handle, float value)
{
    m_command_queue.queue_command("set_grid_density", [this, handle, value]() {
        light_probe_grid_component* comp = m_manager.get_component<light_probe_grid_component>(handle);
        if (comp)
        {
            engine& engine = m_manager.get_world().get_engine();
            render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

            comp->density = value;
            render_command_queue.set_light_probe_grid_density(comp->render_id, value);
        }
    });
}

void light_probe_grid_system::component_removed(object handle, component* comp)
{
    light_probe_grid_component* component = dynamic_cast<light_probe_grid_component*>(comp);
    if (!component)
    {
        return;
    }
    
    render_object_id render_id = component->render_id;
    if (!render_id)
    {
        return;
    }

    m_command_queue.queue_command("destroy_light_probe_grid", [this, render_id]() {
        engine& engine = m_manager.get_world().get_engine();
        render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

        render_command_queue.destroy_light_probe_grid(render_id);
    });
}

void light_probe_grid_system::component_modified(object handle, component* comp, component_modification_source source)
{
    light_probe_grid_component* component = dynamic_cast<light_probe_grid_component*>(comp);
    if (!component)
    {
        return;
    }

    component->is_dirty = true;
}

void light_probe_grid_system::step(const frame_time& time)
{
    engine& engine = m_manager.get_world().get_engine();
    renderer& render = engine.get_renderer();
    render_command_queue& render_command_queue = render.get_command_queue();

    component_filter<light_probe_grid_component, const transform_component> filter(m_manager);
    for (size_t i = 0; i < filter.size(); i++)
    {
        object obj = filter.get_object(i);

        const transform_component* transform = filter.get_component<transform_component>(i);
        light_probe_grid_component* light = filter.get_component<light_probe_grid_component>(i);

        // Create render object if it doesn't exist yet.
        if (light->render_id == null_render_object)
        {
            light->render_id = render_command_queue.create_light_probe_grid("Light Probe Grid");
            light->is_dirty = true;
        }

        // Apply changes if dirty.
        if (light->is_dirty)
        {
            render_command_queue.set_light_probe_grid_density(light->render_id, light->density);
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
