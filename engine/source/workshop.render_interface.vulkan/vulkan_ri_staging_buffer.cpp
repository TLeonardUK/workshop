// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_staging_buffer.h"

namespace ws {

vulkan_ri_staging_buffer::vulkan_ri_staging_buffer(const create_params& params, std::span<uint8_t> linear_data)
    : m_params(params)
{
}

bool vulkan_ri_staging_buffer::is_staged()
{
    return true;
}

void vulkan_ri_staging_buffer::wait()
{
}

}; // namespace ws
