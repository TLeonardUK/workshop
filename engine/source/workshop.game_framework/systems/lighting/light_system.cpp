// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/lighting/light_system.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/lighting/directional_light_component.h"
#include "workshop.game_framework/components/lighting/point_light_component.h"
#include "workshop.game_framework/components/lighting/spot_light_component.h"
#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.renderer/renderer.h"

namespace ws {

light_system::light_system(object_manager& manager, const char* name)
    : system(manager, name)
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

}; // namespace ws
