// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface/ri_types.h"

#include <array>

namespace ws {

size_t ri_bytes_for_data_type(ri_data_type value)
{    
    static std::array<size_t, static_cast<int>(ri_data_type::COUNT)> conversion = {
        1,  // t_bool,
        4,  // t_int,
        4,  // t_uint,
        2,  // t_half,
        4,  // t_float,
        8,  // t_double,

        2,  // t_bool2,
        8,  // t_int2,
        8,  // t_uint2,
        4,  // t_half2,
        8,  // t_float2,
        16, // t_double2,

        3,  // t_bool3,
        12, // t_int3,
        12, // t_uint3,
        6,  // t_half3,
        12, // t_float3,
        24, // t_double3,

        4,  // t_bool4,
        16, // t_int4,
        16, // t_uint4,
        8,  // t_half4,
        16, // t_float4,
        32, // t_double4,

        16,  // t_float2x2,
        32,  // t_double2x2,
        26,  // t_float3x3,
        72,  // t_double3x3,
        64,  // t_float4x4,
        128, // t_double4x4,

        // Resources below are all 4 bytes as they are expected to be converted
        // into uint descriptor table indexes.

        4, // t_texture1d,
        4, // t_texture2d,
        4, // t_texture3d,
        4, // t_texturecube,

        4, // t_sampler,
        4, // t_byteaddressbuffer,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, 0llu, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_data_type: %llu", index);
        return conversion[0];
    }
}

bool ri_is_format_depth_target(ri_texture_format format)
{
    return  format == ri_texture_format::D16_UNORM ||
            format == ri_texture_format::D24_UNORM_S8_UINT ||
            format == ri_texture_format::D32_FLOAT;
}

size_t ri_bytes_per_texel(ri_texture_format value)
{
    static std::array<size_t, static_cast<int>(ri_texture_format::COUNT)> conversion = {
        0, // Undefined,

        32, // R32G32B32A32_FLOAT,
        32, // R32G32B32A32_UINT,
        32, // R32G32B32A32_SINT,

        12, // R32G32B32_FLOAT,
        12, // R32G32B32_UINT,
        12, // R32G32B32_SINT,

        8, // R16G16B16A16_FLOAT,
        8, // R16G16B16A16_UNORM,
        8, // R16G16B16A16_UINT,
        8, // R16G16B16A16_SNORM,
        8, // R16G16B16A16_SINT,

        8, // R32G32_FLOAT,
        8, // R32G32_UINT,
        8, // R32G32_SINT,

        4, // R8G8B8A8_UNORM,
        4, // R8G8B8A8_UNORM_SRGB,
        4, // R8G8B8A8_UINT,
        4, // R8G8B8A8_SNORM,
        4, // R8G8B8A8_SINT,

        4, // R16G16_FLOAT,
        4, // R16G16_UNORM,
        4, // R16G16_UINT,
        4, // R16G16_SNORM,
        4, // R16G16_SINT,

        4, // R32_FLOAT,
        4, // R32_UINT,
        4, // R32_SINT,

        4, // D32_FLOAT,
        4, // D24_UNORM_S8_UINT,

        2, // R8G8_UNORM,
        2, // R8G8_UINT,
        2, // R8G8_SNORM,
        2, // R8G8_SINT,

        2, // R16_FLOAT,
        2, // D16_UNORM,
        2, // R16_UNORM,
        2, // R16_UINT,
        2, // R16_SNORM,
        2, // R16_SINT,

        1, // R8_UNORM,
        1, // R8_UINT,
        1, // R8_SNORM,
        1, // R8_SINT,

        1, // BC1_UNORM,                // These are all compressed formats and need to be handled differently.
        1, // BC1_UNORM_SRGB,
        1, // BC2_UNORM,
        1, // BC2_UNORM_SRGB,
        1, // BC3_UNORM,
        1, // BC3_UNORM_SRGB,
        1, // BC4_UNORM,
        1, // BC4_SNORM,
        1, // BC5_UNORM,
        1, // BC5_SNORM,
        1, // BC6H_UF16,
        1, // BC6H_SF16,
        1, // BC7_UNORM,
        1  // BC7_UNORM_SRGB,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, 0llu, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_texture_format: %llu", index);
        return conversion[0];
    }
}

}; // namespace ws
