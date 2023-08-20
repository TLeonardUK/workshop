// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_system.h"
#include "workshop.renderer/renderer.h"

namespace ws {

render_system::render_system(renderer& render, const char* in_name)
    : m_renderer(render)
    , name(in_name)
{
}

std::vector<render_system*> render_system::get_dependencies()
{
    return m_dependencies;
}

void render_system::add_dependency(const std::type_info& type_info)
{
    render_system* dep = m_renderer.get_system(type_info);
    db_assert(dep != nullptr);
    m_dependencies.push_back(dep);
}

}; // namespace ws
