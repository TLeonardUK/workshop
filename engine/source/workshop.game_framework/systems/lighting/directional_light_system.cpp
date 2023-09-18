// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/lighting/directional_light_system.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/lighting/directional_light_component.h"
#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.game_framework/systems/lighting/light_system.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.renderer/renderer.h"

#pragma optimize("", off)

namespace ws {

directional_light_system::directional_light_system(object_manager& manager)
    : system(manager, "directional light system")
{
    // We want the latest transform to apply to the render object.
    add_predecessor<transform_system>();

    // Light system depends on the render ids we create so always run it after.
    add_successor<light_system>();
}

void directional_light_system::component_removed(object handle, component* comp)
{
    directional_light_component* component = dynamic_cast<directional_light_component*>(comp);
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

        render_command_queue.destroy_directional_light(render_id);
    });
}

void directional_light_system::component_modified(object handle, component* comp, component_modification_source source)
{
    directional_light_component* component = dynamic_cast<directional_light_component*>(comp);
    if (!component)
    {
        return;
    }

    component->is_dirty = true;
}

void directional_light_system::set_light_shadow_cascades(object handle, size_t shadow_cascades)
{
    m_command_queue.queue_command("set_light_shadow_cascades", [this, handle, shadow_cascades]() {
        directional_light_component* comp = m_manager.get_component<directional_light_component>(handle);
        light_component* light_comp = m_manager.get_component<light_component>(handle);
        if (comp && light_comp)
        {
            engine& engine = m_manager.get_world().get_engine();
            render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

            comp->shadow_cascades = shadow_cascades;
            render_command_queue.set_directional_light_shadow_cascades(light_comp->render_id, shadow_cascades);
        }
    });
}

void directional_light_system::set_light_shadow_cascade_exponent(object handle, float value)
{
    m_command_queue.queue_command("set_light_shadow_cascade_exponent", [this, handle, value]() {
        directional_light_component* comp = m_manager.get_component<directional_light_component>(handle);
        light_component* light_comp = m_manager.get_component<light_component>(handle);
        if (comp && light_comp)
        {
            engine& engine = m_manager.get_world().get_engine();
            render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

            comp->shadow_cascade_exponent = value;
            render_command_queue.set_directional_light_shadow_cascade_exponent(light_comp->render_id, value);
        }
    });
}

void directional_light_system::set_light_shadow_cascade_blend(object handle, float value)
{
    m_command_queue.queue_command("set_light_shadow_cascade_blend", [this, handle, value]() {
        directional_light_component* comp = m_manager.get_component<directional_light_component>(handle);
        light_component* light_comp = m_manager.get_component<light_component>(handle);
        if (comp && light_comp)
        {
            engine& engine = m_manager.get_world().get_engine();
            render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

            comp->shadow_cascade_blend = value;
            render_command_queue.set_directional_light_shadow_cascade_blend(light_comp->render_id, value);
        }
    });
}

void directional_light_system::step(const frame_time& time)
{
    engine& engine = m_manager.get_world().get_engine();
    renderer& render = engine.get_renderer();
    render_command_queue& render_command_queue = render.get_command_queue();

    component_filter<directional_light_component, light_component, const transform_component> filter(m_manager);
    for (size_t i = 0; i < filter.size(); i++)
    {
        object obj = filter.get_object(i);

        light_component* light = filter.get_component<light_component>(i);
        const transform_component* transform = filter.get_component<transform_component>(i);
        directional_light_component* directional_light = filter.get_component<directional_light_component>(i);

        // Create render object if it doesn't exist yet.
        if (light->render_id == null_render_object)
        {
            light->render_id = render_command_queue.create_directional_light("Light");

            light->is_dirty = true;
            directional_light->is_dirty = true;
        }

        // Apply changes if dirty.
        if (directional_light->is_dirty)
        {
            render_command_queue.set_directional_light_shadow_cascades(light->render_id, directional_light->shadow_cascades);
            render_command_queue.set_directional_light_shadow_cascade_exponent(light->render_id, directional_light->shadow_cascade_exponent);
            render_command_queue.set_directional_light_shadow_cascade_blend(light->render_id, directional_light->shadow_cascade_blend);

            directional_light->is_dirty = false;
        }
    }

    // Execute all commands after creating the render objects.
    flush_command_queue();
}

}; // namespace ws
