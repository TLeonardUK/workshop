// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface.vulkan/vulkan_headers.h"
#include "workshop.render_interface/ri_types.h"
#include "workshop.core/utils/result.h"

namespace ws {

class vulkan_render_interface;

// ================================================================================================
//  The descriptor tables take a chunk of allocations out of one of the descriptor heap's and
//  sub-allocate them out to anything that asks for them.
//
//  Each table allocates for a specific resource-type. These descriptor tables are then bound to
//  the different unbound arrays in our shaders.
//
//  They are essentially a single bindless array of resources.
// ================================================================================================
class vulkan_ri_descriptor_table
{
public:
    struct allocation
    {
        bool is_valid = false;
        size_t index = 0;
    };

public:
    vulkan_ri_descriptor_table(vulkan_render_interface& renderer, ri_descriptor_table table_type);
    ~vulkan_ri_descriptor_table();

    result<void> create_resources();

    allocation allocate();
    void free(allocation alloc);

    void write_sampled_image(allocation alloc, VkImageView view);
    void write_storage_image(allocation alloc, VkImageView view);
    void write_storage_buffer(allocation alloc, VkBuffer buffer, size_t offset, size_t size);
    void write_sampler(allocation alloc, VkSampler sampler);
    void write_acceleration_structure(allocation alloc, VkAccelerationStructureKHR structure);

private:
    ri_descriptor_table m_table_type;

};

}; // namespace ws
