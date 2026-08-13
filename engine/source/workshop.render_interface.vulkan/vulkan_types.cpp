// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_types.h"
#include "workshop.core/debug/debug.h"
#include "workshop.core/math/math.h"

#include <array>

namespace ws {

namespace {

// Stages shared by shader-visible resources that may be read from any stage except the
// pixel/fragment shader (mirrors D3D12's D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE).
// Deliberately excludes tessellation/geometry shader stages - this engine never uses those
// pipeline stages, and their device features aren't enabled, so including them here would
// make every barrier touching a non-pixel-shader resource reference unsupported stages.
constexpr VkPipelineStageFlags2 k_non_pixel_shader_stages =
    VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
    VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
    VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;

// Stages that may read/write an unordered-access (storage image/buffer) resource.
constexpr VkPipelineStageFlags2 k_unordered_access_stages =
    VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
    VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
    VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
    VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;

}; // namespace

vulkan_image_state ri_to_vulkan_image_state(ri_resource_state value)
{
    static std::array<vulkan_image_state, static_cast<int>(ri_resource_state::COUNT)> conversion = {
        vulkan_image_state{ VK_IMAGE_LAYOUT_UNDEFINED,                     VK_ACCESS_2_NONE,                                    VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT },                                          // initial
        vulkan_image_state{ VK_IMAGE_LAYOUT_GENERAL,                       VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT },                              // common_state
        vulkan_image_state{ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,      VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,              VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT },                              // render_target
        vulkan_image_state{ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,               VK_ACCESS_2_NONE,                                    VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT },                                       // present
        vulkan_image_state{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,          VK_ACCESS_2_TRANSFER_WRITE_BIT,                      VK_PIPELINE_STAGE_2_TRANSFER_BIT },                                             // copy_dest
        vulkan_image_state{ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,          VK_ACCESS_2_TRANSFER_READ_BIT,                       VK_PIPELINE_STAGE_2_TRANSFER_BIT },                                             // copy_source
        vulkan_image_state{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,          VK_ACCESS_2_TRANSFER_WRITE_BIT,                      VK_PIPELINE_STAGE_2_TRANSFER_BIT },                                             // resolve_dest
        vulkan_image_state{ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,          VK_ACCESS_2_TRANSFER_READ_BIT,                       VK_PIPELINE_STAGE_2_TRANSFER_BIT },                                             // resolve_source
        vulkan_image_state{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,      VK_ACCESS_2_SHADER_READ_BIT,                         VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT },                                      // pixel_shader_resource
        vulkan_image_state{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,      VK_ACCESS_2_SHADER_READ_BIT,                         k_non_pixel_shader_stages },                                                    // non_pixel_shader_resource
        vulkan_image_state{ VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,   VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT }, // depth_write
        vulkan_image_state{ VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT }, // depth_read
        vulkan_image_state{ VK_IMAGE_LAYOUT_GENERAL,                       VK_ACCESS_2_MEMORY_READ_BIT,                         VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT },                                         // index_buffer (unused for images)
        vulkan_image_state{ VK_IMAGE_LAYOUT_GENERAL,                       VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, k_unordered_access_stages },                                          // unordered_access
        vulkan_image_state{ VK_IMAGE_LAYOUT_GENERAL,                       VK_ACCESS_2_MEMORY_READ_BIT,                         VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT },                                         // raytracing_acceleration_structure (unused for images)
        vulkan_image_state{ VK_IMAGE_LAYOUT_GENERAL,                       VK_ACCESS_2_MEMORY_READ_BIT,                         VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT },                                         // generic_read
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, size_t{0}, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_resource_state: %llu", index);
        return conversion[0];
    }
}

void ri_record_image_barrier(VkCommandBuffer command_buffer, VkImage image, ri_resource_state source_state, ri_resource_state destination_state, VkImageAspectFlags aspect, uint32_t base_mip, uint32_t mip_count)
{
    vulkan_image_state src = ri_to_vulkan_image_state(source_state);
    vulkan_image_state dst = ri_to_vulkan_image_state(destination_state);

    VkImageMemoryBarrier2 barrier_info = {};
    barrier_info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier_info.srcStageMask = src.stage;
    barrier_info.srcAccessMask = src.access;
    barrier_info.dstStageMask = dst.stage;
    barrier_info.dstAccessMask = dst.access;
    barrier_info.oldLayout = src.layout;
    barrier_info.newLayout = dst.layout;
    barrier_info.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier_info.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier_info.image = image;
    barrier_info.subresourceRange.aspectMask = aspect;
    barrier_info.subresourceRange.baseMipLevel = base_mip;
    barrier_info.subresourceRange.levelCount = mip_count;
    barrier_info.subresourceRange.baseArrayLayer = 0;
    barrier_info.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    VkDependencyInfo dependency_info = {};
    dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &barrier_info;

    vkCmdPipelineBarrier2(command_buffer, &dependency_info);
}

vulkan_buffer_state ri_to_vulkan_buffer_state(ri_resource_state value)
{
    static std::array<vulkan_buffer_state, static_cast<int>(ri_resource_state::COUNT)> conversion = {
        vulkan_buffer_state{ VK_ACCESS_2_NONE,                                                            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT },                          // initial
        vulkan_buffer_state{ VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,                  VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT },                         // common_state
        vulkan_buffer_state{ VK_ACCESS_2_MEMORY_READ_BIT,                                                 VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT },                         // render_target (unused for buffers)
        vulkan_buffer_state{ VK_ACCESS_2_MEMORY_READ_BIT,                                                 VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT },                         // present (unused for buffers)
        vulkan_buffer_state{ VK_ACCESS_2_TRANSFER_WRITE_BIT,                                              VK_PIPELINE_STAGE_2_TRANSFER_BIT },                             // copy_dest
        vulkan_buffer_state{ VK_ACCESS_2_TRANSFER_READ_BIT,                                               VK_PIPELINE_STAGE_2_TRANSFER_BIT },                             // copy_source
        vulkan_buffer_state{ VK_ACCESS_2_TRANSFER_WRITE_BIT,                                              VK_PIPELINE_STAGE_2_TRANSFER_BIT },                             // resolve_dest (unused for buffers)
        vulkan_buffer_state{ VK_ACCESS_2_TRANSFER_READ_BIT,                                               VK_PIPELINE_STAGE_2_TRANSFER_BIT },                             // resolve_source (unused for buffers)
        vulkan_buffer_state{ VK_ACCESS_2_SHADER_READ_BIT,                                                 VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT },                      // pixel_shader_resource
        vulkan_buffer_state{ VK_ACCESS_2_SHADER_READ_BIT,                                                 k_non_pixel_shader_stages },                                    // non_pixel_shader_resource
        vulkan_buffer_state{ VK_ACCESS_2_MEMORY_READ_BIT,                                                 VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT },                         // depth_write (unused for buffers)
        vulkan_buffer_state{ VK_ACCESS_2_MEMORY_READ_BIT,                                                 VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT },                         // depth_read (unused for buffers)
        vulkan_buffer_state{ VK_ACCESS_2_INDEX_READ_BIT,                                                  VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT },                          // index_buffer
        vulkan_buffer_state{ VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,                  k_unordered_access_stages },                                    // unordered_access
        vulkan_buffer_state{ VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR }, // raytracing_acceleration_structure
        vulkan_buffer_state{ VK_ACCESS_2_MEMORY_READ_BIT,                                                 VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT },                         // generic_read
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, size_t{0}, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_resource_state: %llu", index);
        return conversion[0];
    }
}

VkPrimitiveTopology ri_to_vulkan(ri_topology value)
{
    // Note: ri_topology is a coarse "topology class" (used for pipeline state), whereas
    // ri_primitive (below) is the specific primitive list/strip type used for draws.
    // Vulkan doesn't have a separate "topology class" concept like D3D12 does, so this
    // just maps to a representative primitive of that class.
    static std::array<VkPrimitiveTopology, static_cast<int>(ri_topology::COUNT)> conversion = {
        VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
        VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, size_t{0}, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_topology: %llu", index);
        return conversion[0];
    }
}

VkPolygonMode ri_to_vulkan(ri_fill_mode value)
{
    static std::array<VkPolygonMode, static_cast<int>(ri_fill_mode::COUNT)> conversion = {
        VK_POLYGON_MODE_LINE,
        VK_POLYGON_MODE_FILL,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, size_t{0}, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_fill_mode: %llu", index);
        return conversion[0];
    }
}

VkCullModeFlags ri_to_vulkan(ri_cull_mode value)
{
    static std::array<VkCullModeFlags, static_cast<int>(ri_cull_mode::COUNT)> conversion = {
        VK_CULL_MODE_NONE,
        VK_CULL_MODE_BACK_BIT,
        VK_CULL_MODE_FRONT_BIT,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, size_t{0}, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_cull_mode: %llu", index);
        return conversion[0];
    }
}

VkBlendOp ri_to_vulkan(ri_blend_op value)
{
    static std::array<VkBlendOp, static_cast<int>(ri_blend_op::COUNT)> conversion = {
        VK_BLEND_OP_ADD,
        VK_BLEND_OP_SUBTRACT,
        VK_BLEND_OP_REVERSE_SUBTRACT,
        VK_BLEND_OP_MIN,
        VK_BLEND_OP_MAX,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, size_t{0}, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_blend_op: %llu", index);
        return conversion[0];
    }
}

VkBlendFactor ri_to_vulkan(ri_blend_operand value)
{
    static std::array<VkBlendFactor, static_cast<int>(ri_blend_operand::COUNT)> conversion = {
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_SRC_COLOR,
        VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
        VK_BLEND_FACTOR_SRC_ALPHA,
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        VK_BLEND_FACTOR_DST_COLOR,
        VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
        VK_BLEND_FACTOR_DST_ALPHA,
        VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
        VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
        VK_BLEND_FACTOR_CONSTANT_COLOR,
        VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
        VK_BLEND_FACTOR_SRC1_COLOR,
        VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
        VK_BLEND_FACTOR_SRC1_ALPHA,
        VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, size_t{0}, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_blend_operand: %llu", index);
        return conversion[0];
    }
}

VkCompareOp ri_to_vulkan(ri_compare_op value)
{
    static std::array<VkCompareOp, static_cast<int>(ri_compare_op::COUNT)> conversion = {
        VK_COMPARE_OP_NEVER,
        VK_COMPARE_OP_LESS,
        VK_COMPARE_OP_EQUAL,
        VK_COMPARE_OP_LESS_OR_EQUAL,
        VK_COMPARE_OP_GREATER,
        VK_COMPARE_OP_NOT_EQUAL,
        VK_COMPARE_OP_GREATER_OR_EQUAL,
        VK_COMPARE_OP_ALWAYS,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, size_t{0}, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_compare_op: %llu", index);
        return conversion[0];
    }
}

VkStencilOp ri_to_vulkan(ri_stencil_op value)
{
    static std::array<VkStencilOp, static_cast<int>(ri_stencil_op::COUNT)> conversion = {
        VK_STENCIL_OP_KEEP,
        VK_STENCIL_OP_ZERO,
        VK_STENCIL_OP_REPLACE,
        VK_STENCIL_OP_INCREMENT_AND_CLAMP,
        VK_STENCIL_OP_DECREMENT_AND_CLAMP,
        VK_STENCIL_OP_INVERT,
        VK_STENCIL_OP_INCREMENT_AND_WRAP,
        VK_STENCIL_OP_DECREMENT_AND_WRAP,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, size_t{0}, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_stencil_op: %llu", index);
        return conversion[0];
    }
}

VkFormat ri_to_vulkan(ri_texture_format value)
{
    static std::array<VkFormat, static_cast<int>(ri_texture_format::COUNT)> conversion = {
        VK_FORMAT_UNDEFINED,

        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_FORMAT_R32G32B32A32_UINT,
        VK_FORMAT_R32G32B32A32_SINT,

        VK_FORMAT_R32G32B32_SFLOAT,
        VK_FORMAT_R32G32B32_UINT,
        VK_FORMAT_R32G32B32_SINT,

        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R16G16B16A16_UNORM,
        VK_FORMAT_R16G16B16A16_UINT,
        VK_FORMAT_R16G16B16A16_SNORM,
        VK_FORMAT_R16G16B16A16_SINT,

        VK_FORMAT_R32G32_SFLOAT,
        VK_FORMAT_R32G32_UINT,
        VK_FORMAT_R32G32_SINT,

        VK_FORMAT_B10G11R11_UFLOAT_PACK32,

        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_R8G8B8A8_UINT,
        VK_FORMAT_R8G8B8A8_SNORM,
        VK_FORMAT_R8G8B8A8_SINT,

        VK_FORMAT_R16G16_SFLOAT,
        VK_FORMAT_R16G16_UNORM,
        VK_FORMAT_R16G16_UINT,
        VK_FORMAT_R16G16_SNORM,
        VK_FORMAT_R16G16_SINT,

        VK_FORMAT_R32_SFLOAT,
        VK_FORMAT_R32_UINT,
        VK_FORMAT_R32_SINT,

        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,

        VK_FORMAT_R8G8_UNORM,
        VK_FORMAT_R8G8_UINT,
        VK_FORMAT_R8G8_SNORM,
        VK_FORMAT_R8G8_SINT,

        VK_FORMAT_R16_SFLOAT,
        VK_FORMAT_D16_UNORM,
        VK_FORMAT_R16_UNORM,
        VK_FORMAT_R16_UINT,
        VK_FORMAT_R16_SNORM,
        VK_FORMAT_R16_SINT,

        VK_FORMAT_R8_UNORM,
        VK_FORMAT_R8_UINT,
        VK_FORMAT_R8_SNORM,
        VK_FORMAT_R8_SINT,

        VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
        VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
        VK_FORMAT_BC2_UNORM_BLOCK,
        VK_FORMAT_BC2_SRGB_BLOCK,
        VK_FORMAT_BC3_UNORM_BLOCK,
        VK_FORMAT_BC3_SRGB_BLOCK,
        VK_FORMAT_BC4_UNORM_BLOCK,
        VK_FORMAT_BC4_SNORM_BLOCK,
        VK_FORMAT_BC5_UNORM_BLOCK,
        VK_FORMAT_BC5_SNORM_BLOCK,
        VK_FORMAT_BC6H_UFLOAT_BLOCK,
        VK_FORMAT_BC6H_SFLOAT_BLOCK,
        VK_FORMAT_BC7_UNORM_BLOCK,
        VK_FORMAT_BC7_SRGB_BLOCK,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, size_t{0}, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_texture_format: %llu", index);
        return conversion[0];
    }
}

VkFilter ri_to_vulkan_min_mag_filter(ri_texture_filter value)
{
    static std::array<VkFilter, static_cast<int>(ri_texture_filter::COUNT)> conversion = {
        VK_FILTER_LINEAR,    // linear
        VK_FILTER_LINEAR,    // anisotropic
        VK_FILTER_NEAREST,   // nearest_neighbour
        VK_FILTER_LINEAR,    // bilinear
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, size_t{0}, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_texture_filter: %llu", index);
        return conversion[0];
    }
}

VkSamplerMipmapMode ri_to_vulkan_mip_filter(ri_texture_filter value)
{
    static std::array<VkSamplerMipmapMode, static_cast<int>(ri_texture_filter::COUNT)> conversion = {
        VK_SAMPLER_MIPMAP_MODE_LINEAR,  // linear
        VK_SAMPLER_MIPMAP_MODE_LINEAR,  // anisotropic
        VK_SAMPLER_MIPMAP_MODE_NEAREST, // nearest_neighbour
        VK_SAMPLER_MIPMAP_MODE_NEAREST, // bilinear (bilinear filtering within a mip, nearest between mips)
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, size_t{0}, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_texture_filter: %llu", index);
        return conversion[0];
    }
}

bool ri_to_vulkan_is_anisotropic(ri_texture_filter value)
{
    return value == ri_texture_filter::anisotropic;
}

VkSamplerAddressMode ri_to_vulkan(ri_texture_address_mode value)
{
    static std::array<VkSamplerAddressMode, static_cast<int>(ri_texture_address_mode::COUNT)> conversion = {
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, size_t{0}, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_texture_address_mode: %llu", index);
        return conversion[0];
    }
}

color ri_to_vulkan(ri_texture_border_color value)
{
    static std::array<color, static_cast<int>(ri_texture_border_color::COUNT)> conversion = {
        color(0.0f, 0.0f, 0.0f, 0.0f),
        color(1.0f, 1.0f, 1.0f, 0.0f),
        color(0.0f, 0.0f, 0.0f, 1.0f),
        color(1.0f, 1.0f, 1.0f, 1.0f)
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, size_t{0}, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_texture_border_color: %llu", index);
        return conversion[0];
    }
}

VkImageType ri_to_vulkan_image_type(ri_texture_dimension value)
{
    static std::array<VkImageType, static_cast<int>(ri_texture_dimension::COUNT)> conversion = {
        VK_IMAGE_TYPE_1D,
        VK_IMAGE_TYPE_2D,
        VK_IMAGE_TYPE_3D,
        VK_IMAGE_TYPE_2D, // Cube is a 2d array with the cube-compatible flag set.
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, size_t{0}, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_texture_dimension: %llu", index);
        return conversion[0];
    }
}

VkImageViewType ri_to_vulkan_view_type(ri_texture_dimension value)
{
    static std::array<VkImageViewType, static_cast<int>(ri_texture_dimension::COUNT)> conversion = {
        VK_IMAGE_VIEW_TYPE_1D,
        VK_IMAGE_VIEW_TYPE_2D,
        VK_IMAGE_VIEW_TYPE_3D,
        VK_IMAGE_VIEW_TYPE_CUBE,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, size_t{0}, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_texture_dimension: %llu", index);
        return conversion[0];
    }
}

VkPrimitiveTopology ri_to_vulkan(ri_primitive value)
{
    static std::array<VkPrimitiveTopology, static_cast<int>(ri_primitive::COUNT)> conversion = {
        VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
        VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
        VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, size_t{0}, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_primitive: %llu", index);
        return conversion[0];
    }
}

}; // namespace ws
