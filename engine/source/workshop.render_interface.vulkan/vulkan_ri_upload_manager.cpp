// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_upload_manager.h"

namespace ws {

vulkan_ri_upload_manager::vulkan_ri_upload_manager(vulkan_render_interface& renderer)
{
}

vulkan_ri_upload_manager::~vulkan_ri_upload_manager()
{ 
}

result<void> vulkan_ri_upload_manager::create_resources()
{
    return true;
}

void vulkan_ri_upload_manager::new_frame(size_t index)
{ 
}

void vulkan_ri_upload_manager::upload(vulkan_ri_texture& texture, std::span<uint8_t> data)
{
}

void vulkan_ri_upload_manager::upload(vulkan_ri_texture& texture, size_t array_index, size_t mip_index, std::span<uint8_t> data)
{
}

void vulkan_ri_upload_manager::upload(vulkan_ri_texture& texture, size_t array_index, size_t mip_index, vulkan_ri_staging_buffer& data_buffer)
{
}

void vulkan_ri_upload_manager::upload(vulkan_ri_buffer& buffer, std::span<uint8_t> data, size_t offset)
{
}

}; // namespace ws
