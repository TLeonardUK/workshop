// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/engine/world.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.core/perf/profile.h"

#include "workshop.engine/ecs/component_filter.h"

#include "workshop.game_framework/components/camera/camera_component.h"

#include "workshop.physics_interface/physics_interface.h"
#include "workshop.physics_interface/pi_world.h"

namespace ws {

world::world(engine& engine)
    : m_engine(engine)
{
    m_object_manager = std::make_unique<object_manager>(*this);

    pi_world::create_params params;
    params.collision_types = {
        //                  id              collides_with                overlaps_with
        pi_collision_type { "dynamic"_sh, { "static"_sh, "dynamic"_sh }, {} },
        pi_collision_type { "static"_sh,  { },                           {} },
    };

    m_pi_world = m_engine.get_physics_interface().create_world(params, "physics world");
}

engine& world::get_engine()
{
    return m_engine;
}

const char* world::get_name()
{
    return m_name.c_str();
}

object_manager& world::get_object_manager()
{
    return *m_object_manager;
}

// TODO: Nuke this whole thing, the one place we use it (billboards) we should be doing the calculations
// on the gpu anyway so it works per-view.
object world::get_primary_camera()
{
    component_filter<camera_component> filter(*m_object_manager);
    for (size_t i = 0; i < filter.size(); i++)
    {
        object cam_object = filter.get_object(i);
        camera_component* cam_comp = filter.get_component<camera_component>(i);

        // TODO: Do enabled checks when we actually have an enabled state for the camera ...
        return cam_object;
    }
    return null_object;
}

void world::step(const frame_time& time)
{
    profile_marker(profile_colors::engine, "world: %s", m_name.c_str());

    // Don't update this world if stepping has been disabled. This can be because
    // we are in the processing of saving/loading this scene.
    if (!m_step_enabled)
    {
        return;
    }

    bool in_editor = (m_engine.get_editor().get_editor_mode() == editor_mode::editor);

    {
        profile_marker(profile_colors::simulation, "object manager step");
        m_object_manager->step(time, in_editor);
    }

    {
        profile_marker(profile_colors::simulation, "physics step");
        m_pi_world->step(time);
    }
}

void world::set_step_enabled(bool enabled)
{
    m_step_enabled = enabled;
}

pi_world& world::get_physics_world()
{
    return *m_pi_world;
}

}; // namespace ws
