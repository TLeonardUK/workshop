// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_query.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.render_interface.vulkan/vulkan_ri_command_list.h"

namespace ws {

vulkan_ri_query::vulkan_ri_query(vulkan_render_interface& renderer, const char* debug_name, const create_params& params)
    : m_renderer(renderer)
    , m_params(params)
    , m_debug_name(debug_name ? debug_name : "")
{
}

vulkan_ri_query::~vulkan_ri_query()
{
    m_renderer.get_query_manager().free_query(m_query_id);
}

result<void> vulkan_ri_query::create_resources()
{
    m_query_id = m_renderer.get_query_manager().allocate_query();
    return true;
}

const char* vulkan_ri_query::get_debug_name()
{
    return m_debug_name.c_str();
}

bool vulkan_ri_query::are_results_ready()
{
    return m_renderer.get_query_manager().are_results_ready(m_query_id);
}

double vulkan_ri_query::get_results()
{
    return m_renderer.get_query_manager().get_results(m_query_id);
}

void vulkan_ri_query::begin(vulkan_ri_command_list& list)
{
    m_renderer.get_query_manager().begin(m_query_id, list.get_command_buffer());
}

void vulkan_ri_query::end(vulkan_ri_command_list& list)
{
    m_renderer.get_query_manager().end(m_query_id, list.get_command_buffer());
}

}; // namespace ws
