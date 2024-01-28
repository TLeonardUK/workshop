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
    set_flags(system_flags::run_in_editor);

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

    m_command_queue.queue_command("destroy_light", [this, render_id = light_comp->render_id, range_render_id = component->range_render_id]() {
        engine& engine = m_manager.get_world().get_engine();
        render_command_queue& render_command_queue = engine.get_renderer().get_command_queue();

        if (range_render_id)
        {
            render_command_queue.destroy_static_mesh(range_render_id);
        }
        if (render_id)
        {
            render_command_queue.destroy_point_light(render_id);
        }
    });

    component->range_render_id = null_render_object;
    light_comp->render_id = null_render_object;
}

void point_light_system::component_modified(object handle, component* comp, component_modification_source source)
{
    if (dynamic_cast<light_component*>(comp) == nullptr &&
        dynamic_cast<point_light_component*>(comp) == nullptr)
    {
        return;
    }

    light_component* light_comp = m_manager.get_component<light_component>(handle);
    if (!light_comp)
    {
        return;
    }

    light_comp->is_dirty = true;
}

void point_light_system::step(const frame_time& time)
{
    engine& engine = m_manager.get_world().get_engine();
    renderer& render = engine.get_renderer();
    render_command_queue& render_command_queue = render.get_command_queue();

    component_filter<point_light_component, const transform_component, light_component, const meta_component> filter(m_manager);
    for (size_t i = 0; i < filter.size(); i++)
    {
        object obj = filter.get_object(i);

        light_component* light = filter.get_component<light_component>(i);
        const transform_component* transform = filter.get_component<transform_component>(i);
        const meta_component* meta = filter.get_component<meta_component>(i);
        point_light_component* point_light = filter.get_component<point_light_component>(i);

        // Create range display for light.
        if (point_light->range_render_id == null_render_object)
        {
            point_light->range_render_id = render_command_queue.create_static_mesh("Light Range");
            render_command_queue.set_static_mesh_model(point_light->range_render_id, render.get_debug_model(debug_model::sphere));
            render_command_queue.set_static_mesh_materials(point_light->range_render_id, { render.get_debug_material(debug_material::transparent_red) });
            render_command_queue.set_object_gpu_flags(point_light->range_render_id, render_gpu_flags::unlit);
            render_command_queue.set_object_draw_flags(point_light->range_render_id, render_draw_flags::editor);
        }

        // Create render object if it doesn't exist yet.
        if (light->render_id == null_render_object)
        {
            light->render_id = render_command_queue.create_point_light("Light");
            light->is_dirty = true;
        }

        // Apply object transform if its changed.
        if (transform->generation != point_light->last_transform_generation || light->is_dirty)
        {
            render_command_queue.set_object_transform(point_light->range_render_id, transform->world_location, transform->world_rotation, vector3(light->range, light->range, light->range) * 2.0f);
            point_light->last_transform_generation = transform->generation;
        }

        // Apply selected state to debug visualization.
        bool is_selected = (meta->flags & object_flags::selected) != object_flags::none;
        bool was_selected = (point_light->last_flags & object_flags::selected) != object_flags::none;
        if (is_selected != was_selected)
        {
            render_command_queue.set_object_visibility(point_light->range_render_id, is_selected);
        }

        point_light->last_flags = meta->flags;
    }

    // Execute all commands after creating the render objects.
    flush_command_queue();
}

}; // namespace ws
