// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.core/debug/debug.h"
#include "workshop.core/math/math.h"

#include <array>

namespace ws {

D3D12_RESOURCE_STATES ri_to_dx12(ri_resource_state value)
{
    static std::array<D3D12_RESOURCE_STATES, static_cast<int>(ri_resource_state::COUNT)> conversion = {
        D3D12_RESOURCE_STATE_COMMON,                            // initial
        D3D12_RESOURCE_STATE_COMMON,                            // common_state
        D3D12_RESOURCE_STATE_RENDER_TARGET,                     // render_target
        D3D12_RESOURCE_STATE_PRESENT,                           // present
        D3D12_RESOURCE_STATE_COPY_DEST,                         // copy_dest
        D3D12_RESOURCE_STATE_COPY_SOURCE,                       // copy_source
        D3D12_RESOURCE_STATE_RESOLVE_DEST,                      // resolve_dest
        D3D12_RESOURCE_STATE_RESOLVE_SOURCE,                    // resolve_source
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,             // pixel_shader_resource
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,         // non_pixel_shader_resource
        D3D12_RESOURCE_STATE_DEPTH_WRITE,                       // depth_write
        D3D12_RESOURCE_STATE_DEPTH_READ,                        // depth_read
        D3D12_RESOURCE_STATE_INDEX_BUFFER,                      // index_buffer
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,                  // unordered_access
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, // raytracing_acceleration_structure
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, 0llu, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_topology: %llu", index);
        return conversion[0];
    }
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE ri_to_dx12(ri_topology value)
{
    static std::array<D3D12_PRIMITIVE_TOPOLOGY_TYPE, static_cast<int>(ri_topology::COUNT)> conversion = {
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, 0llu, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_topology: %llu", index);
        return conversion[0];
    }
}

D3D12_FILL_MODE ri_to_dx12(ri_fill_mode value)
{
    static std::array<D3D12_FILL_MODE, static_cast<int>(ri_fill_mode::COUNT)> conversion = {
        D3D12_FILL_MODE_WIREFRAME,
        D3D12_FILL_MODE_SOLID,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, 0llu, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_fill_mode: %llu", index);
        return conversion[0];
    }
}

D3D12_CULL_MODE ri_to_dx12(ri_cull_mode value)
{
    static std::array<D3D12_CULL_MODE, static_cast<int>(ri_cull_mode::COUNT)> conversion = {
        D3D12_CULL_MODE_NONE,
        D3D12_CULL_MODE_BACK,
        D3D12_CULL_MODE_FRONT,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, 0llu, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_cull_mode: %llu", index);
        return conversion[0];
    }
}

D3D12_BLEND_OP ri_to_dx12(ri_blend_op value)
{
    static std::array<D3D12_BLEND_OP, static_cast<int>(ri_blend_op::COUNT)> conversion = {
        D3D12_BLEND_OP_ADD,
        D3D12_BLEND_OP_SUBTRACT,
        D3D12_BLEND_OP_REV_SUBTRACT,
        D3D12_BLEND_OP_MIN,
        D3D12_BLEND_OP_MAX,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, 0llu, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_blend_op: %llu", index);
        return conversion[0];
    }
}

D3D12_BLEND ri_to_dx12(ri_blend_operand value)
{
    static std::array<D3D12_BLEND, static_cast<int>(ri_blend_operand::COUNT)> conversion = {
        D3D12_BLEND_ZERO,
        D3D12_BLEND_ONE,
        D3D12_BLEND_SRC_COLOR,
        D3D12_BLEND_INV_SRC_COLOR,
        D3D12_BLEND_SRC_ALPHA,
        D3D12_BLEND_INV_SRC_ALPHA,
        D3D12_BLEND_DEST_COLOR,
        D3D12_BLEND_INV_DEST_COLOR,
        D3D12_BLEND_DEST_ALPHA,
        D3D12_BLEND_INV_DEST_ALPHA,
        D3D12_BLEND_SRC_ALPHA_SAT,
        D3D12_BLEND_BLEND_FACTOR,
        D3D12_BLEND_INV_BLEND_FACTOR,
        D3D12_BLEND_SRC1_COLOR,
        D3D12_BLEND_INV_SRC1_COLOR,
        D3D12_BLEND_SRC1_ALPHA,
        D3D12_BLEND_INV_SRC1_ALPHA,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, 0llu, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_blend_operand: %llu", index);
        return conversion[0];
    }
}

D3D12_COMPARISON_FUNC ri_to_dx12(ri_compare_op value)
{
    static std::array<D3D12_COMPARISON_FUNC, static_cast<int>(ri_compare_op::COUNT)> conversion = {
        D3D12_COMPARISON_FUNC_NEVER,
        D3D12_COMPARISON_FUNC_LESS,
        D3D12_COMPARISON_FUNC_EQUAL,
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        D3D12_COMPARISON_FUNC_GREATER,
        D3D12_COMPARISON_FUNC_NOT_EQUAL,
        D3D12_COMPARISON_FUNC_GREATER_EQUAL,
        D3D12_COMPARISON_FUNC_ALWAYS,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, 0llu, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_compare_op: %llu", index);
        return conversion[0];
    }
}

D3D12_STENCIL_OP ri_to_dx12(ri_stencil_op value)
{
    static std::array<D3D12_STENCIL_OP, static_cast<int>(ri_stencil_op::COUNT)> conversion = {
        D3D12_STENCIL_OP_KEEP,
        D3D12_STENCIL_OP_ZERO,
        D3D12_STENCIL_OP_REPLACE,
        D3D12_STENCIL_OP_INCR_SAT,
        D3D12_STENCIL_OP_DECR_SAT,
        D3D12_STENCIL_OP_INVERT,
        D3D12_STENCIL_OP_INCR,
        D3D12_STENCIL_OP_DECR,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, 0llu, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_stencil_op: %llu", index);
        return conversion[0];
    }
}

DXGI_FORMAT ri_to_dx12(ri_texture_format value)
{
    static std::array<DXGI_FORMAT, static_cast<int>(ri_texture_format::COUNT)> conversion = {
        DXGI_FORMAT_UNKNOWN,

        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_UINT,
        DXGI_FORMAT_R32G32B32A32_SINT,

        DXGI_FORMAT_R32G32B32_FLOAT,
        DXGI_FORMAT_R32G32B32_UINT,
        DXGI_FORMAT_R32G32B32_SINT,

        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_FORMAT_R16G16B16A16_UNORM,
        DXGI_FORMAT_R16G16B16A16_UINT,
        DXGI_FORMAT_R16G16B16A16_SNORM,
        DXGI_FORMAT_R16G16B16A16_SINT,

        DXGI_FORMAT_R32G32_FLOAT,
        DXGI_FORMAT_R32G32_UINT,
        DXGI_FORMAT_R32G32_SINT,

        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        DXGI_FORMAT_R8G8B8A8_UINT,
        DXGI_FORMAT_R8G8B8A8_SNORM,
        DXGI_FORMAT_R8G8B8A8_SINT,

        DXGI_FORMAT_R16G16_FLOAT,
        DXGI_FORMAT_R16G16_UNORM,
        DXGI_FORMAT_R16G16_UINT, 
        DXGI_FORMAT_R16G16_SNORM,
        DXGI_FORMAT_R16G16_SINT, 

        DXGI_FORMAT_R32_FLOAT,
        DXGI_FORMAT_R32_UINT,
        DXGI_FORMAT_R32_SINT,

        DXGI_FORMAT_D32_FLOAT,
        DXGI_FORMAT_D24_UNORM_S8_UINT,

        DXGI_FORMAT_R8G8_UNORM,
        DXGI_FORMAT_R8G8_UINT,
        DXGI_FORMAT_R8G8_SNORM,
        DXGI_FORMAT_R8G8_SINT,

        DXGI_FORMAT_R16_FLOAT,
        DXGI_FORMAT_D16_UNORM,
        DXGI_FORMAT_R16_UNORM,
        DXGI_FORMAT_R16_UINT,
        DXGI_FORMAT_R16_SNORM,
        DXGI_FORMAT_R16_SINT,

        DXGI_FORMAT_R8_UNORM,
        DXGI_FORMAT_R8_UINT,
        DXGI_FORMAT_R8_SNORM,
        DXGI_FORMAT_R8_SINT,

        DXGI_FORMAT_BC1_UNORM,
        DXGI_FORMAT_BC1_UNORM_SRGB,
        DXGI_FORMAT_BC2_UNORM,
        DXGI_FORMAT_BC2_UNORM_SRGB,
        DXGI_FORMAT_BC3_UNORM,
        DXGI_FORMAT_BC3_UNORM_SRGB,
        DXGI_FORMAT_BC4_UNORM,
        DXGI_FORMAT_BC4_SNORM,
        DXGI_FORMAT_BC5_UNORM,
        DXGI_FORMAT_BC5_SNORM,
        DXGI_FORMAT_BC6H_UF16,
        DXGI_FORMAT_BC6H_SF16,
        DXGI_FORMAT_BC7_UNORM,
        DXGI_FORMAT_BC7_UNORM_SRGB,
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

D3D12_FILTER ri_to_dx12(ri_texture_filter value)
{
    static std::array<D3D12_FILTER, static_cast<int>(ri_texture_filter::COUNT)> conversion = {
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_FILTER_ANISOTROPIC,
        D3D12_FILTER_MIN_MAG_MIP_POINT,    
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, 0llu, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_texture_filter: %llu", index);
        return conversion[0];
    }
}

D3D12_TEXTURE_ADDRESS_MODE ri_to_dx12(ri_texture_address_mode value)
{
    static std::array<D3D12_TEXTURE_ADDRESS_MODE, static_cast<int>(ri_texture_address_mode::COUNT)> conversion = {
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_BORDER
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, 0llu, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_texture_address_mode: %llu", index);
        return conversion[0];
    }
}

color ri_to_dx12(ri_texture_border_color value)
{
    static std::array<color, static_cast<int>(ri_texture_border_color::COUNT)> conversion = {
        color(0.0f, 0.0f, 0.0f, 0.0f),
        color(1.0f, 1.0f, 1.0f, 0.0f),
        color(0.0f, 0.0f, 0.0f, 1.0f),
        color(1.0f, 1.0f, 1.0f, 1.0f)
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, 0llu, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_texture_border_color: %llu", index);
        return conversion[0];
    }
}

D3D12_RESOURCE_DIMENSION ri_to_dx12(ri_texture_dimension value)
{
    static std::array<D3D12_RESOURCE_DIMENSION, static_cast<int>(ri_texture_dimension::COUNT)> conversion = {
        D3D12_RESOURCE_DIMENSION_TEXTURE1D,
        D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        D3D12_RESOURCE_DIMENSION_TEXTURE3D,
        D3D12_RESOURCE_DIMENSION_TEXTURE2D  // Cube is 2d array.
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, 0llu, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of ri_texture_dimension: %llu", index);
        return conversion[0];
    }
}

D3D12_PRIMITIVE_TOPOLOGY ri_to_dx12(ri_primitive value)
{
    static std::array<D3D12_PRIMITIVE_TOPOLOGY, static_cast<int>(ri_primitive::COUNT)> conversion = {
        D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
        D3D_PRIMITIVE_TOPOLOGY_LINELIST,
        D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, 0llu, conversion.size()))
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
