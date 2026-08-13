// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_query.h"

namespace ws {

vulkan_ri_query::vulkan_ri_query(const create_params& params, const char* debug_name)
    : m_params(params)
    , m_debug_name(debug_name ? debug_name : "")
{
}

const char* vulkan_ri_query::get_debug_name()
{
    return m_debug_name.c_str();
}

bool vulkan_ri_query::are_results_ready()
{
    return true;
}

double vulkan_ri_query::get_results()
{
    return 0.0;
}

}; // namespace ws
