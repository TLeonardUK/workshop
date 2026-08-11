// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_types.h"
#include "workshop.render_interface.vulkan/vulkan_headers.h"

#include "workshop.core/drawing/color.h"

namespace ws {

// ================================================================================================
//  Bundles the layout/access/stage triple required to build a vulkan barrier for a given
//  ri_resource_state. Unlike D3D12, vulkan requires all three of these to be specified
//  independently rather than being implied by a single resource-state enum.
// ================================================================================================
struct vulkan_image_state
{
    VkImageLayout layout;
    VkAccessFlags2 access;
    VkPipelineStageFlags2 stage;
};

struct vulkan_buffer_state
{
    VkAccessFlags2 access;
    VkPipelineStageFlags2 stage;
};

// Records a pipeline barrier transitioning an image between two ri_resource_states. Shared by
// vulkan_ri_command_list::barrier(VkImage, ...) and any other code that needs to record the same
// transition into a command buffer it owns directly (eg. one-off immediate submits).
void ri_record_image_barrier(VkCommandBuffer command_buffer, VkImage image, ri_resource_state source_state, ri_resource_state destination_state, VkImageAspectFlags aspect, uint32_t base_mip = 0, uint32_t mip_count = VK_REMAINING_MIP_LEVELS);

// Convert various RI types to their respective Vulkan types.
vulkan_image_state ri_to_vulkan_image_state(ri_resource_state value);
vulkan_buffer_state ri_to_vulkan_buffer_state(ri_resource_state value);
VkPrimitiveTopology ri_to_vulkan(ri_topology value);
VkPolygonMode ri_to_vulkan(ri_fill_mode value);
VkCullModeFlags ri_to_vulkan(ri_cull_mode value);
VkBlendOp ri_to_vulkan(ri_blend_op value);
VkBlendFactor ri_to_vulkan(ri_blend_operand value);
VkCompareOp ri_to_vulkan(ri_compare_op value);
VkStencilOp ri_to_vulkan(ri_stencil_op value);
VkFormat ri_to_vulkan(ri_texture_format value);
VkFilter ri_to_vulkan_min_mag_filter(ri_texture_filter value);
VkSamplerMipmapMode ri_to_vulkan_mip_filter(ri_texture_filter value);
bool ri_to_vulkan_is_anisotropic(ri_texture_filter value);
VkSamplerAddressMode ri_to_vulkan(ri_texture_address_mode value);
color ri_to_vulkan(ri_texture_border_color value);
VkImageType ri_to_vulkan_image_type(ri_texture_dimension value);
VkImageViewType ri_to_vulkan_view_type(ri_texture_dimension value);
VkPrimitiveTopology ri_to_vulkan(ri_primitive value);

}; // namespace ws
