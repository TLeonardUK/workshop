// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/containers/string.h"

#include "workshop.core/filesystem/stream.h"

namespace ws {

// ================================================================================================
//  Data types for interop with gpu data.
// ================================================================================================
enum class render_data_type
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

    COUNT
};

inline static const char* render_data_type_strings[static_cast<int>(render_data_type::COUNT)] = {
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
};



inline static const char* render_data_type_hlsl_type[static_cast<int>(render_data_type::COUNT)] = {
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
};

DEFINE_ENUM_TO_STRING(render_data_type, render_data_type_strings)

// ================================================================================================
//  Topology of vertex data passed into vertex shader.
// ================================================================================================
enum class render_topology
{
    point,
    line,
    triangle,
    patch,

    COUNT
};

inline static const char* render_topology_strings[static_cast<int>(render_topology::COUNT)] = {
    "point",
    "line",
    "triangle",
    "patch",
};

DEFINE_ENUM_TO_STRING(render_topology, render_topology_strings)

// ================================================================================================
//  Defines how primitives are filled.
// ================================================================================================
enum class render_fill_mode
{
    wireframe,
    solid,

    COUNT
};

inline static const char* render_fill_mode_strings[static_cast<int>(render_fill_mode::COUNT)] = {
    "wireframe",
    "solid"
};

DEFINE_ENUM_TO_STRING(render_fill_mode, render_fill_mode_strings)

// ================================================================================================
//  Defines which faces of a primitive are culled.
// ================================================================================================
enum class render_cull_mode
{
    none,
    back,
    front,

    COUNT
};

inline static const char* render_cull_mode_strings[static_cast<int>(render_cull_mode::COUNT)] = {
    "none",
    "back",
    "front"
};

DEFINE_ENUM_TO_STRING(render_cull_mode, render_cull_mode_strings)

// ================================================================================================
//  Defines the method used to blend together the source and destination colors of a blend op.
// ================================================================================================
enum class render_blend_op
{
    add,
    subtract,
    reverse_subtract,
    min,
    max,

    COUNT
};

inline static const char* render_blend_op_strings[static_cast<int>(render_blend_op::COUNT)] = {
    "add",
    "subtract",
    "reverse_subtract",
    "min",
    "max"
};

DEFINE_ENUM_TO_STRING(render_blend_op, render_blend_op_strings)

// ================================================================================================
//  Defines the method used to blend together the source and destination colors of a blend op.
// ================================================================================================
enum class render_blend_operand
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

inline static const char* render_blend_operand_strings[static_cast<int>(render_blend_operand::COUNT)] = {
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

DEFINE_ENUM_TO_STRING(render_blend_operand, render_blend_operand_strings)

// ================================================================================================
//  Defines the comparison operator used for various rendering comparisons.
// ================================================================================================
enum class render_compare_op
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

inline static const char* render_compare_op_strings[static_cast<int>(render_compare_op::COUNT)] = {
    "never",
    "less",
    "equal",
    "less_equal",
    "greater",
    "not_equal",
    "greater_equal",
    "always"
};

DEFINE_ENUM_TO_STRING(render_compare_op, render_compare_op_strings)

// ================================================================================================
//  Defines the operator used for various stencil operations.
// ================================================================================================
enum class render_stencil_op
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

inline static const char* render_stencil_op_strings[static_cast<int>(render_stencil_op::COUNT)] = {
    "keep",
    "zero",
    "replace",
    "increase_saturated",
    "decrease_saturated",
    "inverse",
    "increase",
    "decrease",
};

DEFINE_ENUM_TO_STRING(render_stencil_op, render_stencil_op_strings)

// ================================================================================================
//  State of the graphics pipeline at the point a draw call is made.
// ================================================================================================
struct render_pipeline_state
{
    // Raster state
    render_topology         topology;
    render_fill_mode        fill_mode;
    render_cull_mode        cull_mode;
    uint32_t                depth_bias;
    float                   depth_bias_clamp;
    float                   slope_scaled_depth_bias;
    bool                    depth_clip_enabled;
    bool                    multisample_enabled;
    bool                    antialiased_line_enabled;
    bool                    conservative_raster_enabled;

    // Blend state
    bool                    alpha_to_coverage;
    bool                    blend_enabled;
    render_blend_op         blend_op;
    render_blend_operand    blend_source_op;
    render_blend_operand    blend_destination_op;
    render_blend_op         blend_alpha_op;
    render_blend_operand    blend_alpha_source_op;
    render_blend_operand    blend_alpha_destination_op;

    // Depth state
    bool                    depth_test_enabled;
    bool                    depth_write_enabled;
    render_compare_op       depth_compare_op;

    // Stencil state
    bool                    stencil_test_enabled;
    uint32_t                stencil_read_mask;
    uint32_t                stencil_write_mask;
    render_stencil_op       stencil_front_face_fail_op;
    render_stencil_op       stencil_front_face_depth_fail_op;
    render_stencil_op       stencil_front_face_pass_op;
    render_compare_op       stencil_front_face_compare_op;
    render_stencil_op       stencil_back_face_fail_op;
    render_stencil_op       stencil_back_face_depth_fail_op;
    render_stencil_op       stencil_back_face_pass_op;
    render_compare_op       stencil_back_face_compare_op;

};

template<> 
inline void stream_serialize(stream& out, render_pipeline_state& state)
{
    stream_serialize_enum(out, state.topology);
    stream_serialize_enum(out, state.fill_mode);
    stream_serialize_enum(out, state.cull_mode);
    stream_serialize(out, state.depth_bias);
    stream_serialize(out, state.depth_bias_clamp);
    stream_serialize(out, state.slope_scaled_depth_bias);
    stream_serialize(out, state.depth_clip_enabled);
    stream_serialize(out, state.multisample_enabled);
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
enum class render_shader_stage
{
    vertex,
    pixel,
    domain,
    hull,
    geometry,
    compute,

    COUNT
};

inline static const char* render_shader_stage_strings[static_cast<int>(render_shader_stage::COUNT)] = {
    "vertex",
    "pixel",
    "domain",
    "hull",
    "geometry",
    "compute"
};

DEFINE_ENUM_TO_STRING(render_shader_stage, render_shader_stage_strings)

}; // namespace ws
