// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/containers/string.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/geometry/geometry.h"

#include "workshop.core/drawing/pixmap.h"
#include "workshop.core/hashing/hash.h"

namespace ws {

// ================================================================================================
//  Used to reference a give face on a cube map.
// ================================================================================================
enum class ri_cube_map_face
{
    x_pos,
    x_neg,
    y_pos,
    y_neg,
    z_pos,
    z_neg
};

// ================================================================================================
//  Describes the current access-state of a resource on the gpu.
// ================================================================================================
enum class ri_resource_state
{
    // This is a special state, it returns the resource to whatever state it expects
    // to be in between command lists (defined by the resource). Parallel generation of 
    // command lists should always return the resource to this state before finishing.
    initial,

    // Common state for sharing resource between multiple gpu engines.
    common_state,

    // Resource is usable as a render target.
    render_target,

    // Resource is only usable for presentation.
    present,

    // Destination for copying data to.
    copy_dest,

    // Source for copying data from.
    copy_source,

    // Destination for resolve operation.
    resolve_dest,

    // Source for resolve operation.
    resolve_source,

    // Resource used for pixel shader.
    pixel_shader_resource,

    // Resource used for non-pixel shader.
    non_pixel_shader_resource,

    // For writing depth to.
    depth_write,

    // For reading depth from.
    depth_read,

    // For storing index buffers.
    index_buffer,

    // For reading in shaders as a UAV.
    unordered_access,

    COUNT
};

// ================================================================================================
//  Data types for interop with gpu data.
// ================================================================================================
enum class ri_data_type
{
    t_bool,
    t_int,
    t_uint,
    t_half,
    t_float,
    t_double,

    t_bool2,
    t_int2,
    t_uint2,
    t_half2,
    t_float2,
    t_double2,

    t_bool3,
    t_int3,
    t_uint3,
    t_half3,
    t_float3,
    t_double3,

    t_bool4,
    t_int4,
    t_uint4,
    t_half4,
    t_float4,
    t_double4,

    t_float2x2,
    t_double2x2,
    t_float3x3,
    t_double3x3,
    t_float4x4,
    t_double4x4,

    t_texture1d,
    t_texture2d,
    t_texture3d,
    t_texturecube,

    t_sampler,
    t_byteaddressbuffer,
    t_rwbyteaddressbuffer,
    t_rwtexture2d,

    COUNT
};

inline static const char* ri_data_type_strings[static_cast<int>(ri_data_type::COUNT)] = {
    "bool",
    "int",
    "uint",
    "half",
    "float",
    "double",

    "bool2",
    "int2",
    "uint2",
    "half2",
    "float2",
    "double2",

    "bool3",
    "int3",
    "uint3",
    "half3",
    "float3",
    "double3",

    "bool4",
    "int4",
    "uint4",
    "half4",
    "float4",
    "double4",

    "float2x2",
    "double2x2",
    "float3x3",
    "double3x3",
    "float4x4",
    "double4x4",

    "texture1d",
    "texture2d",
    "texture3d",
    "texturecube",

    "sampler",
    "byteaddressbuffer",
    "rwbyteaddressbuffer",
    "rwtexture2d",
};

inline static const char* ri_data_type_hlsl_type[static_cast<int>(ri_data_type::COUNT)] = {
    "bool",
    "int",
    "uint",
    "half",
    "float",
    "double",

    "bool2",
    "int2",
    "uint2",
    "half2",
    "float2",
    "double2",

    "bool3",
    "int3",
    "uint3",
    "half3",
    "float3",
    "double3",

    "bool4",
    "int4",
    "uint4",
    "half4",
    "float4",
    "double4",

    "float2x2",
    "double2x2",
    "float3x3",
    "double3x3",
    "float4x4",
    "double4x4",

    "Texture1D",
    "Texture2D",
    "Texture3D",
    "TextureCube",

    "sampler",
    "ByteAddressBuffer",
    "RWByteAddressBuffer",
    "RWTexture2D"
};

DEFINE_ENUM_TO_STRING(ri_data_type, ri_data_type_strings)

// Number of bytes a given data type takes up on the gpu.
size_t ri_bytes_for_data_type(ri_data_type type);

// Converts a geometry type to an ri data type.
ri_data_type ri_convert_geometry_data_type(geometry_data_type type);

// ================================================================================================
//  Topology of vertex data as interpreted by hull/geometry shaders.
// ================================================================================================
enum class ri_topology
{
    point,
    line,
    triangle,
    patch,

    COUNT
};

inline static const char* ri_topology_strings[static_cast<int>(ri_topology::COUNT)] = {
    "point",
    "line",
    "triangle",
    "patch",
};

DEFINE_ENUM_TO_STRING(ri_topology, ri_topology_strings)

// ================================================================================================
//  Descriptor table type
// ================================================================================================
enum class ri_descriptor_table
{
    texture_1d,
    texture_2d,
    texture_3d,
    texture_cube,
    sampler,
    buffer,
    rwbuffer,
    rwtexture_2d,

    render_target,
    depth_stencil,

    COUNT
};

inline static const char* ri_descriptor_table_strings[static_cast<int>(ri_descriptor_table::COUNT)] = {

    "texture_1d",
    "texture_2d",
    "texture_3d",
    "texture_cube",
    "sampler",
    "buffer",
    "rwbuffer",
    "rwtexture_2d",

    "render_target",
    "depth_stencil"
};

DEFINE_ENUM_TO_STRING(ri_descriptor_table, ri_descriptor_table_strings)

// ================================================================================================
//  Type of primitive data an index buffer represents.
// ================================================================================================
enum class ri_primitive
{
    point_list,
    line_list,
    line_strip,
    triangle_list,
    triangle_strip,

    COUNT
};

inline static const char* ri_primitive_strings[static_cast<int>(ri_primitive::COUNT)] = {
    "point_list",
    "line_list",
    "line_strip",
    "triangle_list",
    "triangle_strip," 
};

DEFINE_ENUM_TO_STRING(ri_primitive, ri_primitive_strings)

// ================================================================================================
//  Defines how primitives are filled.
// ================================================================================================
enum class ri_fill_mode
{
    wireframe,
    solid,

    COUNT
};

inline static const char* ri_fill_mode_strings[static_cast<int>(ri_fill_mode::COUNT)] = {
    "wireframe",
    "solid"
};

DEFINE_ENUM_TO_STRING(ri_fill_mode, ri_fill_mode_strings)

// ================================================================================================
//  Defines which faces of a primitive are culled.
// ================================================================================================
enum class ri_cull_mode
{
    none,
    back,
    front,

    COUNT
};

inline static const char* ri_cull_mode_strings[static_cast<int>(ri_cull_mode::COUNT)] = {
    "none",
    "back",
    "front"
};

DEFINE_ENUM_TO_STRING(ri_cull_mode, ri_cull_mode_strings)

// ================================================================================================
//  Defines the method used to blend together the source and destination colors of a blend op.
// ================================================================================================
enum class ri_blend_op
{
    add,
    subtract,
    reverse_subtract,
    min,
    max,

    COUNT
};

inline static const char* ri_blend_op_strings[static_cast<int>(ri_blend_op::COUNT)] = {
    "add",
    "subtract",
    "reverse_subtract",
    "min",
    "max"
};

DEFINE_ENUM_TO_STRING(ri_blend_op, ri_blend_op_strings)

// ================================================================================================
//  Defines the method used to blend together the source and destination colors of a blend op.
// ================================================================================================
enum class ri_blend_operand
{
    zero,
    one,
    source_color,
    inverse_source_color,
    source_alpha,
    inverse_source_alpha,
    destination_color,
    inverse_destination_color,
    destination_alpha,
    inverse_destination_alpha,
    source_alpha_saturated,
    blend_factor,
    inverse_blend_factor,
    source1_color,
    inverse_source1_color,
    source1_alpha,
    inverse_source1_alpha,

    COUNT
};

inline static const char* ri_blend_operand_strings[static_cast<int>(ri_blend_operand::COUNT)] = {
    "zero",
    "one",
    "source_color",
    "inverse_source_color",
    "source_alpha",
    "inverse_source_alpha",
    "destination_color",
    "inverse_destination_color",
    "destination_alpha",
    "inverse_destination_alpha",
    "source_alpha_saturated",
    "blend_factor",
    "inverse_blend_factor",
    "source1_color",
    "inverse_source1_color",
    "source1_alpha",
    "inverse_source1_alpha"
};

DEFINE_ENUM_TO_STRING(ri_blend_operand, ri_blend_operand_strings)

// ================================================================================================
//  Defines the comparison operator used for various rendering comparisons.
// ================================================================================================
enum class ri_compare_op
{
    never,
    less,
    equal,
    less_equal,
    greater,
    not_equal,
    greater_equal,
    always,

    COUNT
};

inline static const char* ri_compare_op_strings[static_cast<int>(ri_compare_op::COUNT)] = {
    "never",
    "less",
    "equal",
    "less_equal",
    "greater",
    "not_equal",
    "greater_equal",
    "always"
};

DEFINE_ENUM_TO_STRING(ri_compare_op, ri_compare_op_strings)

// ================================================================================================
//  Defines the operator used for various stencil operations.
// ================================================================================================
enum class ri_stencil_op
{
    keep,
    zero,
    replace,
    increase_saturated,
    decrease_saturated,
    inverse,
    increase,
    decrease,

    COUNT
};

inline static const char* ri_stencil_op_strings[static_cast<int>(ri_stencil_op::COUNT)] = {
    "keep",
    "zero",
    "replace",
    "increase_saturated",
    "decrease_saturated",
    "inverse",
    "increase",
    "decrease",
};

DEFINE_ENUM_TO_STRING(ri_stencil_op, ri_stencil_op_strings)

// ================================================================================================
//  State of the graphics pipeline at the point a draw call is made.
// ================================================================================================
struct ri_pipeline_render_state
{
    // Raster state
    ri_topology         topology;
    ri_fill_mode        fill_mode;
    ri_cull_mode        cull_mode;
    uint32_t            depth_bias;
    float               depth_bias_clamp;
    float               slope_scaled_depth_bias;
    bool                depth_clip_enabled;
    bool                multisample_enabled;
    uint32_t            multisample_count;
    bool                antialiased_line_enabled;
    bool                conservative_raster_enabled;

    // Blend state
    bool                alpha_to_coverage;
    bool                blend_enabled;
    ri_blend_op         blend_op;
    ri_blend_operand    blend_source_op;
    ri_blend_operand    blend_destination_op;
    ri_blend_op         blend_alpha_op;
    ri_blend_operand    blend_alpha_source_op;
    ri_blend_operand    blend_alpha_destination_op;

    // Depth state
    bool                depth_test_enabled;
    bool                depth_write_enabled;
    ri_compare_op       depth_compare_op;

    // Stencil state
    bool                stencil_test_enabled;
    uint32_t            stencil_read_mask;
    uint32_t            stencil_write_mask;
    ri_stencil_op       stencil_front_face_fail_op;
    ri_stencil_op       stencil_front_face_depth_fail_op;
    ri_stencil_op       stencil_front_face_pass_op;
    ri_compare_op       stencil_front_face_compare_op;
    ri_stencil_op       stencil_back_face_fail_op;
    ri_stencil_op       stencil_back_face_depth_fail_op;
    ri_stencil_op       stencil_back_face_pass_op;
    ri_compare_op       stencil_back_face_compare_op;

};

template<> 
inline void stream_serialize(stream& out, ri_pipeline_render_state& state)
{
    stream_serialize_enum(out, state.topology);
    stream_serialize_enum(out, state.fill_mode);
    stream_serialize_enum(out, state.cull_mode);
    stream_serialize(out, state.depth_bias);
    stream_serialize(out, state.depth_bias_clamp);
    stream_serialize(out, state.slope_scaled_depth_bias);
    stream_serialize(out, state.depth_clip_enabled);
    stream_serialize(out, state.multisample_enabled);
    stream_serialize(out, state.multisample_count);
    stream_serialize(out, state.antialiased_line_enabled);
    stream_serialize(out, state.conservative_raster_enabled);

    stream_serialize(out, state.alpha_to_coverage);
    stream_serialize(out, state.blend_enabled);
    stream_serialize_enum(out, state.blend_op);
    stream_serialize_enum(out, state.blend_source_op);
    stream_serialize_enum(out, state.blend_destination_op);
    stream_serialize_enum(out, state.blend_alpha_op);
    stream_serialize_enum(out, state.blend_alpha_source_op);
    stream_serialize_enum(out, state.blend_alpha_destination_op);

    stream_serialize(out, state.depth_test_enabled);
    stream_serialize(out, state.depth_write_enabled);
    stream_serialize_enum(out, state.depth_compare_op);

    stream_serialize(out, state.stencil_test_enabled);
    stream_serialize(out, state.stencil_read_mask);
    stream_serialize(out, state.stencil_write_mask);
    stream_serialize_enum(out, state.stencil_front_face_fail_op);
    stream_serialize_enum(out, state.stencil_front_face_depth_fail_op);
    stream_serialize_enum(out, state.stencil_front_face_pass_op);
    stream_serialize_enum(out, state.stencil_front_face_compare_op);
    stream_serialize_enum(out, state.stencil_back_face_fail_op);
    stream_serialize_enum(out, state.stencil_back_face_depth_fail_op);
    stream_serialize_enum(out, state.stencil_back_face_pass_op);
    stream_serialize_enum(out, state.stencil_back_face_compare_op);
}

// ================================================================================================
//  Defines the different shader stages
// ================================================================================================
enum class ri_shader_stage
{
    vertex,
    pixel,
    domain,
    hull,
    geometry,
    compute,

    COUNT
};

inline static const char* ri_shader_stage_strings[static_cast<int>(ri_shader_stage::COUNT)] = {
    "vertex",
    "pixel",
    "domain",
    "hull",
    "geometry",
    "compute"
};

DEFINE_ENUM_TO_STRING(ri_shader_stage, ri_shader_stage_strings)

// ================================================================================================
//  Defines different data formats for textures.
// ================================================================================================
enum class ri_texture_format
{
    Undefined,

    R32G32B32A32_FLOAT,
    R32G32B32A32,
    R32G32B32A32_SINT,

    R32G32B32_FLOAT,
    R32G32B32,
    R32G32B32_SINT,

    R16G16B16A16_FLOAT,
    R16G16B16A16,
    R16G16B16A16_UINT,
    R16G16B16A16_SNORM,
    R16G16B16A16_SINT,

    R32G32_FLOAT,
    R32G32,
    R32G32_SINT,

    R8G8B8A8,
    R8G8B8A8_SRGB,
    R8G8B8A8_UINT,
    R8G8B8A8_SNORM,
    R8G8B8A8_SINT,

    R16G16_FLOAT,
    R16G16,
    R16G16_UINT,
    R16G16_SNORM,
    R16G16_SINT,

    R32_FLOAT,
    R32,
    R32_SINT,

    D32_FLOAT,
    D24_UNORM_S8_UINT,

    R8G8,
    R8G8_UINT,
    R8G8_SNORM,
    R8G8_SINT,

    R16_FLOAT,
    D16,
    R16,
    R16_UINT,
    R16_SNORM,
    R16_SINT,

    R8,
    R8_UINT,
    R8_SNORM,
    R8_SINT,

    BC1,
    BC1_SRGB,
    BC2,
    BC2_SRGB,
    BC3,
    BC3_SRGB,
    BC4,
    BC4_SIGNED,
    BC5,
    BC5_SIGNED,
    BC6H_UF16,
    BC6H_SF16,
    BC7,
    BC7_SRGB,

    COUNT
};

inline static const char* ri_texture_format_strings[static_cast<int>(ri_texture_format::COUNT)] = {
    "Undefined",
    
    "R32G32B32A32_FLOAT",
    "R32G32B32A32",
    "R32G32B32A32_SINT",

    "R32G32B32_FLOAT",
    "R32G32B32",
    "R32G32B32_SINT",

    "R16G16B16A16_FLOAT",
    "R16G16B16A16",
    "R16G16B16A16_UINT",
    "R16G16B16A16_SNORM",
    "R16G16B16A16_SINT",

    "R32G32_FLOAT",
    "R32G32",
    "R32G32_SINT",

    "R8G8B8A8",
    "R8G8B8A8_SRGB",
    "R8G8B8A8_UINT",
    "R8G8B8A8_SNORM",
    "R8G8B8A8_SINT",

    "R16G16_FLOAT",
    "R16G16",
    "R16G16_UINT",
    "R16G16_SNORM",
    "R16G16_SINT",

    "R32_FLOAT",
    "R32",
    "R32_SINT",

    "D32_FLOAT",
    "D24_UNORM_S8_UINT",

    "R8G8",
    "R8G8_UINT",
    "R8G8_SNORM",
    "R8G8_SINT",

    "R16_FLOAT",
    "D16",
    "R16",
    "R16_UINT",
    "R16_SNORM",
    "R16_SINT",

    "R8",
    "R8_UINT",
    "R8_SNORM",
    "R8_SINT",

    "BC1",
    "BC1_SRGB",
    "BC2",
    "BC2_SRGB",
    "BC3",
    "BC3_SRGB",
    "BC4",
    "BC4_SIGNED",
    "BC5",
    "BC5_SIGNED",
    "BC6H_UF16",
    "BC6H_SF16",
    "BC7",
    "BC7_SRGB",
};

DEFINE_ENUM_TO_STRING(ri_texture_format, ri_texture_format_strings)

// Determines if a texture format is suitable for a depth-stencil render target.
bool ri_is_format_depth_target(ri_texture_format format);

// Determines how many bytes each texel takes up.
size_t ri_bytes_per_texel(ri_texture_format format);

// Gets the block size of a compressed format, or 1 if uncompressed.
size_t ri_format_block_size(ri_texture_format format);

// Converts a pixmap format into the equivilent ri texture format.
ri_texture_format ri_convert_pixmap_format(pixmap_format format);

// ================================================================================================
//  Defines the layout of a data block. 
//  This is a generic container for things like vertex buffers, param blocks, etc.
// ================================================================================================
struct ri_data_layout
{
    struct field
    {
        std::string name;
        ri_data_type data_type;
    };

    std::vector<field> fields;

    bool operator!=(const ri_data_layout& other) const
    {
        return !operator==(other);
    }

    bool operator==(const ri_data_layout& other) const
    {
        if (fields.size() != other.fields.size())
        {
            return false;
        }

        for (size_t i = 0; i < fields.size(); i++)
        {
            const field& this_field = fields[i];
            const field& other_field = other.fields[i];

            if (this_field.name != other_field.name ||
                this_field.data_type != other_field.data_type)
            {
                return false;
            }
        }

        return true;
    }
};


// ================================================================================================
//  Defines the scope of usage of an arbitrary data block.
//  This is a generic value used to define how things like param blocks are instanced.
// ================================================================================================
enum class ri_data_scope
{
    // Exists globally, managed by code.
    global,

    // Exists for a specific draw call. Multiple instances of the same model will share 
    // the same param block.
    draw,
    
    // Exists for each instance of a draw. Keep size as minimal as possible to make instanced
    // rendering as efficient as possible.
    instance,

    COUNT
};

inline static const char* ri_data_scope_strings[static_cast<int>(ri_data_scope::COUNT)] = {
    "global",
    "draw",
    "instance"
};

DEFINE_ENUM_TO_STRING(ri_data_scope, ri_data_scope_strings)

// ================================================================================================
//  Defines the number of dimensions a teture has.
// ================================================================================================
enum class ri_texture_dimension
{
    texture_1d,
    texture_2d,
    texture_3d,
    texture_cube,

    COUNT
};

inline static const char* ri_texture_dimension_strings[static_cast<int>(ri_texture_dimension::COUNT)] = {
    "1d",
    "2d",
    "3d",
    "cube" 
};

DEFINE_ENUM_TO_STRING(ri_texture_dimension, ri_texture_dimension_strings)

// ================================================================================================
//  Defines a texture filtering mode.
// ================================================================================================
enum class ri_texture_filter
{
    linear,
    anisotropic,
    nearest_neighbour,

    COUNT
};

inline static const char* ri_texture_filter_strings[static_cast<int>(ri_texture_filter::COUNT)] = {
    "linear",
    "anisotropic",
    "nearest_neighbour"
};

DEFINE_ENUM_TO_STRING(ri_texture_filter, ri_texture_filter_strings)

// ================================================================================================
//  Defines a texture addressing mode.
// ================================================================================================
enum class ri_texture_address_mode
{
    repeat,
    mirrored_repeat,
    clamp_to_edge,
    clamp_to_border,

    COUNT
};

inline static const char* ri_texture_address_mode_strings[static_cast<int>(ri_texture_address_mode::COUNT)] = {
    "repeat",
    "mirrored_repeat",
    "clamp_to_edge",
    "clamp_to_border",
};

DEFINE_ENUM_TO_STRING(ri_texture_address_mode, ri_texture_address_mode_strings)

// ================================================================================================
//  Defines a texture border sampling color.
// ================================================================================================
enum class ri_texture_border_color
{
    transparent_black,
    transparent_white,
    opaque_black,
    opaque_white,
    
    COUNT
};

inline static const char* ri_texture_border_color_strings[static_cast<int>(ri_texture_border_color::COUNT)] = {
    "transparent_black",
    "transparent_white",
    "opaque_black",
    "opaque_white",
};

DEFINE_ENUM_TO_STRING(ri_texture_border_color, ri_texture_border_color_strings)

}; // namespace ws

namespace std {

// Hash implementation for ri_data_layout.
template <>
struct hash<ws::ri_data_layout>
{
    std::size_t operator()(const ws::ri_data_layout& k) const
    {
        using std::size_t;
        using std::hash;
        using std::string;

        size_t h = 0;
        ws::hash_combine(h, k.fields.size());

        for (const ws::ri_data_layout::field& field : k.fields)
        {
            ws::hash_combine(h, static_cast<int>(field.data_type));
            ws::hash_combine(h, field.name);
        }

        return h;
    }
};

}; // namespace std