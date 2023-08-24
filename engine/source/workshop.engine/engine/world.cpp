// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/engine/world.h"
#include "workshop.core/perf/profile.h"

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

void world::step(const frame_time& time)
{
    profile_marker(profile_colors::engine, "world: %s", m_name.c_str());

    m_object_manager->step(time);
}

}; // namespace ws
