// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/lighting/spot_light_system.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/lighting/spot_light_component.h"
#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.game_framework/systems/lighting/light_system.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.renderer/renderer.h"

namespace ws {

spot_light_system::spot_light_system(object_manager& manager)
    : system(manager, "spot light system")
{
    // We want the latest transform to apply to the render object.
    add_predecessor<transform_system>();

    // Light system depends on the render ids we create so always run it after.
    add_successor<light_system>();
}

void spot_light_system::component_removed(object handle, component* comp)
{
    spot_light_component* component = dynamic_cast<spot_light_component*>(comp);
    if (!component)
    {
        return;
    }

    light_component* light_comp = m_manager.get_component<light_component>(handle);
    if (!light_comp)
    {
        return;
    }
    
    render_object_id render_id = light_comp->render_id;
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
        light_component* light_comp = m_manager.get_component<light_component>(handle);
        if (comp && light_comp)
        {
            engine& engine = m_manager.get_world().get_engine();
            render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

            comp->inner_radius = inner_radius;
            comp->outer_radius = outer_radius;
            render_command_queue.set_spot_light_radius(light_comp->render_id, inner_radius, outer_radius);
        }
    });
}

void spot_light_system::step(const frame_time& time)
{
    engine& engine = m_manager.get_world().get_engine();
    renderer& render = engine.get_renderer();
    render_command_queue& render_command_queue = render.get_command_queue();

    component_filter<spot_light_component, light_component, const transform_component> filter(m_manager);
    for (size_t i = 0; i < filter.size(); i++)
    {
        object obj = filter.get_object(i);

        light_component* light = filter.get_component<light_component>(i);
        const transform_component* transform = filter.get_component<transform_component>(i);
        spot_light_component* spot_light = filter.get_component<spot_light_component>(i);

        // Create render object if it doesn't exist yet.
        if (light->render_id == null_render_object)
        {
            light->render_id = render_command_queue.create_spot_light("Light");
            light->is_dirty = true;
            spot_light->is_dirty = true;
        }

        // Apply changes if dirty.
        if (spot_light->is_dirty)
        {
            render_command_queue.set_spot_light_radius(light->render_id, spot_light->inner_radius, spot_light->outer_radius);

            spot_light->is_dirty = false;
        }
    }

    // Execute all commands after creating the render objects.
    flush_command_queue();
}

}; // namespace ws
