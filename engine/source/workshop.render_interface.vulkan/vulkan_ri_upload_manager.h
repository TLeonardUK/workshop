// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface.vulkan/vulkan_headers.h"
#include "workshop.core/utils/result.h"

#include <span>

namespace ws {

class vulkan_render_interface;
class vulkan_ri_texture;
class vulkan_ri_buffer;
class vulkan_ri_staging_buffer;

// ================================================================================================
//  Handles copying CPU data to GPU memory.
// ================================================================================================
class vulkan_ri_upload_manager
{
public:
    vulkan_ri_upload_manager(vulkan_render_interface& renderer);
    ~vulkan_ri_upload_manager();

    result<void> create_resources();

    void new_frame(size_t index);

    void upload(vulkan_ri_texture& texture, std::span<uint8_t> data);
    void upload(vulkan_ri_texture& texture, size_t array_index, size_t mip_index, std::span<uint8_t> data);
    void upload(vulkan_ri_texture& texture, size_t array_index, size_t mip_index, vulkan_ri_staging_buffer& data_buffer);
    void upload(vulkan_ri_buffer& buffer, std::span<uint8_t> data, size_t offset);

};

}; // namespace ws
