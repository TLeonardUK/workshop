// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/ecs/system.h"
#include "workshop.engine/ecs/object_manager.h"
#include "workshop.core/debug/log.h"

namespace ws {

system::system(object_manager& manager, const char* name)
    : m_manager(manager)
    , m_name(name)
    , m_command_queue(k_command_queue_capacity)
{
}

std::vector<system*> system::get_dependencies()
{
    return m_dependencies;
}

void system::add_dependency(const std::type_index& type_info, bool predecessor)
{
    system* dep = m_manager.get_system(type_info);
    db_assert(dep != nullptr);

    if (predecessor)
    {
        m_dependencies.push_back(dep);
    }
    else
    {
        dep->m_dependencies.push_back(this);
    }
}

const char* system::get_name()
{
    return m_name.c_str();
}

void system::flush_command_queue()
{
    while (!m_command_queue.empty())
    {
        m_command_queue.execute_next();
    }
    m_command_queue.reset();
}

}; // namespace ws
