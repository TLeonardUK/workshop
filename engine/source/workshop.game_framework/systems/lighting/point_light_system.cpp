// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/lighting/point_light_system.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/lighting/point_light_component.h"
#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.game_framework/systems/lighting/light_system.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.renderer/renderer.h"

namespace ws {

point_light_system::point_light_system(object_manager& manager)
    : system(manager, "ponint light system")
{
    // We want the latest transform to apply to the render object.
    add_predecessor<transform_system>();

    // Light system depends on the render ids we create so always run it after.
    add_successor<light_system>();
}

void point_light_system::component_removed(object handle, component* comp)
{
    point_light_component* component = dynamic_cast<point_light_component*>(comp);
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

        render_command_queue.destroy_point_light(render_id);
    });
}

void point_light_system::component_modified(object handle, component* comp, component_modification_source source)
{
    light_component* component = dynamic_cast<light_component*>(comp);
    if (!component)
    {
        return;
    }

    component->is_dirty = true;
}

void point_light_system::step(const frame_time& time)
{
    engine& engine = m_manager.get_world().get_engine();
    renderer& render = engine.get_renderer();
    render_command_queue& render_command_queue = render.get_command_queue();

    component_filter<point_light_component, const transform_component, light_component> filter(m_manager);
    for (size_t i = 0; i < filter.size(); i++)
    {
        object obj = filter.get_object(i);

        light_component* light = filter.get_component<light_component>(i);
        const transform_component* transform = filter.get_component<transform_component>(i);
        point_light_component* point_light = filter.get_component<point_light_component>(i);

        // Create render object if it doesn't exist yet.
        if (light->render_id == null_render_object)
        {
            light->render_id = render_command_queue.create_point_light("Light");
            light->is_dirty = true;
        }
    }

    // Execute all commands after creating the render objects.
    flush_command_queue();
}

}; // namespace ws
