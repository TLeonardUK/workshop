// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/lighting/light_system.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.engine/ecs/meta_component.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/lighting/directional_light_component.h"
#include "workshop.game_framework/components/lighting/point_light_component.h"
#include "workshop.game_framework/components/lighting/spot_light_component.h"
#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.renderer/renderer.h"

namespace ws {

light_system::light_system(object_manager& manager)
    : system(manager, "light system")
{
}

void light_system::set_light_intensity(object id, float value)
{
    m_command_queue.queue_command("set_light_intensity", [this, id, value]() {
        light_component* comp = m_manager.get_component<light_component>(id);
        if (comp)
        {
            engine& engine = m_manager.get_world().get_engine();
            render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

            comp->intensity = value;
            render_command_queue.set_light_intensity(comp->render_id, value);
        }
    });
}

void light_system::set_light_range(object id, float value)
{
    m_command_queue.queue_command("set_light_range", [this, id, value]() {
        light_component* comp = m_manager.get_component<light_component>(id);
        if (comp)
        {
            engine& engine = m_manager.get_world().get_engine();
            render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

            comp->range = value;
            comp->is_dirty = true;
            render_command_queue.set_light_range(comp->render_id, value);
        }
    });
}

void light_system::set_light_importance_distance(object id, float value)
{
    m_command_queue.queue_command("set_light_importance_distance", [this, id, value]() {
        light_component* comp = m_manager.get_component<light_component>(id);
        if (comp)
        {
            engine& engine = m_manager.get_world().get_engine();
            render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

            comp->importance_range = value;
            render_command_queue.set_light_importance_distance(comp->render_id, value);
        }
    });
}

void light_system::set_light_color(object id, color value)
{
    m_command_queue.queue_command("set_light_color", [this, id, value]() {
        light_component* comp = m_manager.get_component<light_component>(id);
        if (comp)
        {
            engine& engine = m_manager.get_world().get_engine();
            render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

            comp->color = value;
            render_command_queue.set_light_color(comp->render_id, value);
        }
    });
}

void light_system::set_light_shadow_casting(object id, bool value)
{
    m_command_queue.queue_command("set_light_shadow_casting", [this, id, value]() {
        light_component* comp = m_manager.get_component<light_component>(id);
        if (comp)
        {
            engine& engine = m_manager.get_world().get_engine();
            render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

            comp->shadow_casting = value;
            render_command_queue.set_light_shadow_casting(comp->render_id, value);
        }
    });
}

void light_system::set_light_shadow_map_size(object id, size_t value)
{
    m_command_queue.queue_command("set_light_shadow_map_size", [this, id, value]() {
        light_component* comp = m_manager.get_component<light_component>(id);
        if (comp)
        {
            engine& engine = m_manager.get_world().get_engine();
            render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

            comp->shadow_map_size = value;
            render_command_queue.set_light_shadow_map_size(comp->render_id, value);
        }
    });
}

void light_system::set_light_shadow_max_distance(object id, float value)
{
    m_command_queue.queue_command("set_light_shadow_max_distance", [this, id, value]() {
        light_component* comp = m_manager.get_component<light_component>(id);
        if (comp)
        {
            engine& engine = m_manager.get_world().get_engine();
            render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

            comp->shadow_map_distance = value;
            render_command_queue.set_light_shadow_max_distance(comp->render_id, value);
        }
    });
}

void light_system::component_modified(object handle, component* comp, component_modification_source source)
{
    light_component* component = dynamic_cast<light_component*>(comp);
    if (!component)
    {
        return;
    }

    component->is_dirty = true;
}

void light_system::component_removed(object handle, component* comp)
{
    light_component* component = dynamic_cast<light_component*>(comp);
    if (!component)
    {
        return;
    }

    render_object_id render_render_id = component->range_render_id;
    if (!render_render_id)
    {
        return;
    }

    m_command_queue.queue_command("destroy_light", [this, render_render_id]() {
        engine& engine = m_manager.get_world().get_engine();
        render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

        render_command_queue.destroy_static_mesh(render_render_id);
    });
}

void light_system::step(const frame_time& time)
{
    engine& engine = m_manager.get_world().get_engine();
    renderer& render = engine.get_renderer();
    render_command_queue& render_command_queue = render.get_command_queue();

    component_filter<light_component, const transform_component, const meta_component> filter(m_manager);
    for (size_t i = 0; i < filter.size(); i++)
    {
        object obj = filter.get_object(i);

        light_component* light = filter.get_component<light_component>(i);
        const meta_component* meta = filter.get_component<meta_component>(i);
        const transform_component* transform = filter.get_component<transform_component>(i);

        // Create range display for light.
        if (light->range_render_id == null_render_object)
        {
            light->range_render_id = render_command_queue.create_static_mesh("Light Range");
            render_command_queue.set_static_mesh_model(light->range_render_id, render.get_debug_model(debug_model::sphere));
            render_command_queue.set_static_mesh_materials(light->range_render_id, { render.get_debug_material(debug_material::transparent_red) });
            render_command_queue.set_object_gpu_flags(light->range_render_id, render_gpu_flags::unlit);
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
        }        
        
        // Apply object transform if its changed.
        if (transform->generation != light->last_transform_generation || light->is_dirty)
        {
            render_command_queue.set_object_transform(light->render_id, transform->world_location, transform->world_rotation, transform->world_scale);
            render_command_queue.set_object_transform(light->range_render_id, transform->world_location, transform->world_rotation, vector3(light->range, light->range, light->range));

            light->last_transform_generation = transform->generation;
        }

        // Apply selected state to debug visualization.
        bool is_selected = (meta->flags & object_flags::selected) != object_flags::none;
        bool was_selected = (light->last_flags & object_flags::selected) != object_flags::none;
        if (is_selected != was_selected)
        {
            render_command_queue.set_object_visibility(light->range_render_id, is_selected);
        }

        light->last_flags = meta->flags;
        light->is_dirty = false;
    }

    // Execute all commands after creating the render objects.
    flush_command_queue();
}

}; // namespace ws
