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

}; // namespace ws
