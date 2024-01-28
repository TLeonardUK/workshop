// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/engine/world.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.core/perf/profile.h"

#include "workshop.engine/ecs/component_filter.h"

#include "workshop.game_framework/components/camera/camera_component.h"

namespace ws {

world::world(engine& engine)
    : m_engine(engine)
{
    m_object_manager = std::make_unique<object_manager>(*this);
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
    m_object_manager->step(time, in_editor);
}

void world::set_step_enabled(bool enabled)
{
    m_step_enabled = enabled;
}

}; // namespace ws
