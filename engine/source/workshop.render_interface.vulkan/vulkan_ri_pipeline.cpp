// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_pipeline.h"

namespace ws {

vulkan_ri_pipeline::vulkan_ri_pipeline(const create_params& params, const char* debug_name)
    : m_params(params)
{
}

const ri_pipeline::create_params& vulkan_ri_pipeline::get_create_params()
{
    return m_params;
}

}; // namespace ws
