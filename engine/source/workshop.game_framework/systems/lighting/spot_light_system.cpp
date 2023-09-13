// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/lighting/spot_light_system.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/lighting/spot_light_component.h"
#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.renderer/renderer.h"

namespace ws {

spot_light_system::spot_light_system(object_manager& manager)
    : light_system(manager, "spot light system")
{
    // We want the latest transform to apply to the render object.
    add_predecessor<transform_system>();
}

void spot_light_system::component_removed(object handle, component* comp)
{
    spot_light_component* component = dynamic_cast<spot_light_component*>(comp);
    if (!component)
    {
        return;
    }
    
    render_object_id render_id = component->render_id;
    if (!render_id)
    {
        return;
    }

    m_command_queue.queue_command("destroy_light", [this, render_id]() {
        engine& engine = m_manager.get_world().get_engine();
        render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

        render_command_queue.destroy_spot_light(render_id);
    });
}

void spot_light_system::component_modified(object handle, component* comp, component_modification_source source)
{
    spot_light_component* component = dynamic_cast<spot_light_component*>(comp);
    if (!component)
    {
        return;
    }

    component->is_dirty = true;
}

void spot_light_system::set_light_radius(object handle, float inner_radius, float outer_radius)
{
    m_command_queue.queue_command("set_light_radius", [this, handle, inner_radius, outer_radius]() {
        spot_light_component* comp = m_manager.get_component<spot_light_component>(handle);
        if (comp)
        {
            engine& engine = m_manager.get_world().get_engine();
            render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

            comp->inner_radius = inner_radius;
            comp->outer_radius = outer_radius;
            render_command_queue.set_spot_light_radius(comp->render_id, inner_radius, outer_radius);
        }
    });
}

void spot_light_system::step(const frame_time& time)
{
    engine& engine = m_manager.get_world().get_engine();
    renderer& render = engine.get_renderer();
    render_command_queue& render_command_queue = render.get_command_queue();

    component_filter<spot_light_component, const transform_component> filter(m_manager);
    for (size_t i = 0; i < filter.size(); i++)
    {
        object obj = filter.get_object(i);

        const transform_component* transform = filter.get_component<transform_component>(i);
        spot_light_component* light = filter.get_component<spot_light_component>(i);

        // Create render object if it doesn't exist yet.
        if (light->render_id == null_render_object)
        {
            light->render_id = render_command_queue.create_spot_light("Light");
            light->is_dirty = true;
        }

        // Apply changes if dirty.
        if (light->is_dirty)
        {
            render_command_queue.set_light_intensity(light->render_id, light->intensity);
            render_command_queue.set_light_range(light->render_id, light->range);
            render_command_queue.set_light_importance_distance(light->render_id, light->importance_range);
            render_command_queue.set_light_color(light->render_id, light->color);
            render_command_queue.set_light_shadow_casting(light->render_id, light->shadow_casting);
            render_command_queue.set_light_shadow_map_size(light->render_id, light->shadow_map_size);
            render_command_queue.set_light_shadow_max_distance(light->render_id, light->shadow_map_distance);
            render_command_queue.set_spot_light_radius(light->render_id, light->inner_radius, light->outer_radius);
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
