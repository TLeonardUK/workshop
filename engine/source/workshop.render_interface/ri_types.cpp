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
        4,  // t_bool,
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
        4, // t_rwbyteaddressbuffer,
        4, // t_rwtexture2d,

        4, // t_compressed_unit_vector
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

ri_data_type ri_convert_geometry_data_type(geometry_data_type value)
{    
    static std::array<ri_data_type, static_cast<int>(geometry_data_type::COUNT)> conversion = {
        ri_data_type::t_bool, // t_bool,
	    ri_data_type::t_int, // t_int,
	    ri_data_type::t_uint, // t_uint,
	    ri_data_type::t_half, // t_half,
	    ri_data_type::t_float, // t_float,
	    ri_data_type::t_double,  // t_double,

	    ri_data_type::t_bool2, // t_bool2,
	    ri_data_type::t_int2, // t_int2,
	    ri_data_type::t_uint2, // t_uint2,
	    ri_data_type::t_half2, // t_half2,
	    ri_data_type::t_float2, // t_float2,
	    ri_data_type::t_double2, // t_double2,
        
        ri_data_type::t_bool3, // t_bool3,
	    ri_data_type::t_int3, // t_int3,
	    ri_data_type::t_uint3, // t_uint3,
	    ri_data_type::t_half3, // t_half3,
	    ri_data_type::t_float3, // t_float3,
	    ri_data_type::t_double3, // t_double3,
        
        ri_data_type::t_bool4, // t_bool4,
	    ri_data_type::t_int4, // t_int4,
	    ri_data_type::t_uint4, // t_uint4,
	    ri_data_type::t_half4, // t_half4,
	    ri_data_type::t_float4, // t_float4,
	    ri_data_type::t_double4, // t_double4,
        
        ri_data_type::t_float2x2, // t_float2x2,
	    ri_data_type::t_double2x2, // t_double2x2,
	    ri_data_type::t_float3x3, // t_float3x3,
	    ri_data_type::t_double3x3, // t_double3x3,
	    ri_data_type::t_float4x4, // t_float4x4,
	    ri_data_type::t_double4x4, // t_double4x4,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, 0llu, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of geometry_data_type: %llu", index);
        return conversion[0];
    }
}

bool ri_is_format_depth_target(ri_texture_format format)
{
    return  format == ri_texture_format::D16 ||
            format == ri_texture_format::D24_UNORM_S8_UINT ||
            format == ri_texture_format::D32_FLOAT;
}

size_t ri_bytes_per_texel(ri_texture_format value)
{
    static std::array<size_t, static_cast<int>(ri_texture_format::COUNT)> conversion = {
        0, // Undefined,

        16, // R32G32B32A32_FLOAT,
        16, // R32G32B32A32,
        16, // R32G32B32A32_SINT,

        12, // R32G32B32_FLOAT,
        12, // R32G32B32,
        12, // R32G32B32_SINT,

        8, // R16G16B16A16_FLOAT,
        8, // R16G16B16A16,
        8, // R16G16B16A16_UINT,
        8, // R16G16B16A16_SNORM,
        8, // R16G16B16A16_SINT,

        8, // R32G32_FLOAT,
        8, // R32G32,
        8, // R32G32_SINT,

        4, // R8G8B8A8,
        4, // R8G8B8A8_SRGB,
        4, // R8G8B8A8_UINT,
        4, // R8G8B8A8_SNORM,
        4, // R8G8B8A8_SINT,

        4, // R16G16_FLOAT,
        4, // R16G16,
        4, // R16G16_UINT,
        4, // R16G16_SNORM,
        4, // R16G16_SINT,

        4, // R32_FLOAT,
        4, // R32,
        4, // R32_SINT,

        4, // D32_FLOAT,
        4, // D24_UNORM_S8_UINT,

        2, // R8G8,
        2, // R8G8_UINT,
        2, // R8G8_SNORM,
        2, // R8G8_SINT,

        2, // R16_FLOAT,
        2, // D16,
        2, // R16,
        2, // R16_UINT,
        2, // R16_SNORM,
        2, // R16_SINT,

        1, // R8,
        1, // R8_UINT,
        1, // R8_SNORM,
        1, // R8_SINT,

        2, // BC1,                // As these are compressed formats, we treat this as a single row in the encoded block.
        2, // BC1_SRGB,
        2, // BC2,
        2, // BC2_SRGB,
        4, // BC3,
        4, // BC3_SRGB,
        2, // BC4,
        2, // BC4_SIGNED,
        4, // BC5,
        4, // BC5_SIGNED,
        4, // BC6H_UF16,
        4, // BC6H_SF16,
        4, // BC7,
        4  // BC7_SRGB,
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

size_t ri_format_block_size(ri_texture_format value)
{
    static std::array<size_t, static_cast<int>(ri_texture_format::COUNT)> conversion = {
        1, // Undefined,
        
        1, // R32G32B32A32_FLOAT,
        1, // R32G32B32A32,
        1, // R32G32B32A32_SINT,
        
        1, // R32G32B32_FLOAT,
        1, // R32G32B32,
        1, // R32G32B32_SINT,
        
        1, // R16G16B16A16_FLOAT,
        1, // R16G16B16A16,
        1, // R16G16B16A16_UINT,
        1, // R16G16B16A16_SNORM,
        1, // R16G16B16A16_SINT,
        
        1, // R32G32_FLOAT,
        1, // R32G32,
        1, // R32G32_SINT,
        
        1, // R8G8B8A8,
        1, // R8G8B8A8_SRGB,
        1, // R8G8B8A8_UINT,
        1, // R8G8B8A8_SNORM,
        1, // R8G8B8A8_SINT,
        
        1, // R16G16_FLOAT,
        1, // R16G16,
        1, // R16G16_UINT,
        1, // R16G16_SNORM,
        1, // R16G16_SINT,
        
        1, // R32_FLOAT,
        1, // R32,
        1, // R32_SINT,
        
        1, // D32_FLOAT,
        1, // D24_UNORM_S8_UINT,
        
        1, // R8G8,
        1, // R8G8_UINT,
        1, // R8G8_SNORM,
        1, // R8G8_SINT,
        
        1, // R16_FLOAT,
        1, // D16,
        1, // R16,
        1, // R16_UINT,
        1, // R16_SNORM,
        1, // R16_SINT,
        
        1, // R8,
        1, // R8_UINT,
        1, // R8_SNORM,
        1, // R8_SINT,

        4, // BC1,                
        4, // BC1_SRGB,
        4, // BC2,
        4, // BC2_SRGB,
        4, // BC3,
        4, // BC3_SRGB,
        4, // BC4,
        4, // BC4_SIGNED,
        4, // BC5,
        4, // BC5_SIGNED,
        4, // BC6H_UF16,
        4, // BC6H_SF16,
        4, // BC7,
        4  // BC7_SRGB,
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

ri_texture_format ri_convert_pixmap_format(pixmap_format value)
{
    static std::array<ri_texture_format, static_cast<int>(pixmap_format::COUNT)> conversion = {
        ri_texture_format::R32G32B32A32_FLOAT,  // R32G32B32A32_FLOAT,
        ri_texture_format::R32G32B32A32_SINT,   // R32G32B32A32_SIGNED,
        ri_texture_format::R32G32B32A32,        // R32G32B32A32,
        
        ri_texture_format::R32G32B32_FLOAT,     // R32G32B32_FLOAT,
        ri_texture_format::R32G32B32_SINT,      // R32G32B32_SIGNED,
        ri_texture_format::R32G32B32,           // R32G32B32,
        
        ri_texture_format::R32G32_FLOAT,        // R32G32_FLOAT,
        ri_texture_format::R32G32_SINT,         // R32G32_SIGNED,
        ri_texture_format::R32G32,              // R32G32,
        
        ri_texture_format::R32_FLOAT,           // R32_FLOAT,
        ri_texture_format::R32_SINT,            // R32_SIGNED,
        ri_texture_format::R32,                 // R32,
        
        ri_texture_format::R16G16B16A16_FLOAT,  // R16G16B16A16_FLOAT,
        ri_texture_format::R16G16B16A16_SINT,   // R16G16B16A16_SIGNED,
        ri_texture_format::R16G16B16A16,        // R16G16B16A16,
        
        ri_texture_format::R16G16_FLOAT,        // R16G16_FLOAT,
        ri_texture_format::R16G16_SINT,         // R16G16_SIGNED,
        ri_texture_format::R16G16,              // R16G16,
        
        ri_texture_format::R16_FLOAT,           // R16_FLOAT,
        ri_texture_format::R16_SINT,            // R16_SIGNED,
        ri_texture_format::R16,                 // R16,
        
        ri_texture_format::R8G8B8A8_SNORM,      // R8G8B8A8_SIGNED,
        ri_texture_format::R8G8B8A8,            // R8G8B8A8,

        ri_texture_format::R8G8_SNORM,          // R8G8,
        ri_texture_format::R8G8,                // R8G8_SIGNED,
        
        ri_texture_format::R8_SNORM,            // R8,
        ri_texture_format::R8,                  // R8_SIGNED,
        
        ri_texture_format::BC1,                 // BC1,
        ri_texture_format::BC3,                 // BC3,
        ri_texture_format::BC4,                 // BC4,
        ri_texture_format::BC5,                 // BC5,
        ri_texture_format::BC7,                 // BC7,

        ri_texture_format::BC6H_SF16,           // BC6_SF16,
        ri_texture_format::BC6H_UF16            // BC6_UF16,
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
