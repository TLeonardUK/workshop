// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/common/gbuffer.hlsl"
#include "data:shaders/source/common/vertex.hlsl"
#include "data:shaders/source/common/normal.hlsl"
#include "data:shaders/source/common/lighting.hlsl"

// ================================================================================================
//  Standard path
// ================================================================================================

struct geometry_pinput
{
    float4 position : SV_POSITION;
    float2 uv0 : TEXCOORD0;
    float4 world_normal : TEXCOORD1;
    float4 world_tangent : NORMAL1;
    float4 world_position : TEXCOORD3;
    uint flags : TEXCOORD2;
};

geometry_pinput vshader(vertex_input input)
{
    vertex v = load_vertex(input.vertex_id);
    geometry_instance_info vi = load_geometry_instance_info(input.instance_id);

    float4x4 mvp_matrix = mul(projection_matrix, mul(view_matrix, vi.model_matrix));

    geometry_pinput result;
    result.flags = vi.gpu_flags;
    result.position = mul(mvp_matrix, float4(v.position, 1.0f));
    result.uv0 = v.uv0;
    result.world_normal = mul(vi.model_matrix, float4(v.normal, 0.0f));
    result.world_tangent = mul(vi.model_matrix, float4(v.tangent, 0.0f));
    result.world_position = mul(vi.model_matrix, float4(v.position, 1.0f));

    return result;
}

gbuffer_output pshader_common(geometry_pinput input, float4 albedo)
{
    gbuffer_fragment f;
    f.albedo = albedo.rgb;
    f.flags = input.flags;
    f.metallic = metallic_texture.Sample(metallic_sampler, input.uv0).r;
    f.roughness = roughness_texture.Sample(roughness_sampler, input.uv0).r;
    f.world_normal = normalize(input.world_normal).xyz;
    f.world_position = input.world_position;

#ifdef GBUFFER_DEBUG_DATA
    f.debug_data.x = length(f.world_position.xyz - view_world_position);
#endif

    return encode_gbuffer(f);
}

gbuffer_output pshader_opaque(geometry_pinput input)
{
    float4 albedo = albedo_texture.Sample(albedo_sampler, input.uv0);
    return pshader_common(input, albedo);
}

gbuffer_output pshader_masked(geometry_pinput input)
{
    float4 albedo = albedo_texture.Sample(albedo_sampler, input.uv0);
    if (albedo.a < 0.5)
    {
        discard;
    }
    return pshader_common(input, albedo);
}

gbuffer_output pshader_transparent(geometry_pinput input)
{
    float4 albedo = albedo_texture.Sample(albedo_sampler, input.uv0);
    return pshader_common(input, albedo);
}

gbuffer_output pshader_transparent_masked(geometry_pinput input)
{
    float4 albedo = albedo_texture.Sample(albedo_sampler, input.uv0);
    float4 opacity = opacity_texture.Sample(albedo_sampler, input.uv0);
    albedo *= opacity;
    return pshader_common(input, albedo);
}

// ================================================================================================
//  Depth only
// ================================================================================================

struct geometry_pinput_depth_only
{
    float4 position : SV_POSITION;
    float2 uv0 : TEXCOORD0;
};

geometry_pinput_depth_only vshader_depth_only(vertex_input input)
{
    vertex v = load_vertex(input.vertex_id);
    geometry_instance_info vi = load_geometry_instance_info(input.instance_id);

    float4x4 mvp_matrix = mul(projection_matrix, mul(view_matrix, vi.model_matrix));

    geometry_pinput_depth_only result;
    result.position = mul(mvp_matrix, float4(v.position, 1.0f));
    result.uv0 = v.uv0;

    return result;
}

void pshader_opaque_depth_only(geometry_pinput_depth_only input)
{
}

void pshader_masked_depth_only(geometry_pinput_depth_only input)
{
    float4 albedo = albedo_texture.Sample(albedo_sampler, input.uv0);
    if (albedo.a < 0.5)
    {
        discard;
    }
}

// ================================================================================================
//  Linear depth only
// ================================================================================================

struct geometry_pinput_linear_depth_only
{
    float4 position : SV_POSITION;
    float2 uv0 : TEXCOORD0;
    float4 world_position : TEXCOORD3;
};

struct linear_depth_output
{
    float depth : SV_DEPTH;
};

geometry_pinput_linear_depth_only vshader_linear_depth_only(vertex_input input)
{
    vertex v = load_vertex(input.vertex_id);
    geometry_instance_info vi = load_geometry_instance_info(input.instance_id);

    float4x4 mvp_matrix = mul(projection_matrix, mul(view_matrix, vi.model_matrix));

    geometry_pinput_linear_depth_only result;
    result.position = mul(mvp_matrix, float4(v.position, 1.0f));
    result.uv0 = v.uv0;
    result.world_position = mul(vi.model_matrix, float4(v.position, 1.0f));

    return result;
}

linear_depth_output pshader_common_linear_depth_only(geometry_pinput_linear_depth_only input)
{
    float distance = length(input.world_position.xyz - view_world_position);
    distance = distance / view_z_far;
    
    linear_depth_output output;
    output.depth = distance;    
    return output;
}

linear_depth_output pshader_opaque_linear_depth_only(geometry_pinput_linear_depth_only input)
{
    return pshader_common_linear_depth_only(input);
}

linear_depth_output pshader_masked_linear_depth_only(geometry_pinput_linear_depth_only input)
{
    float4 albedo = albedo_texture.Sample(albedo_sampler, input.uv0);
    if (albedo.a < 0.5)
    {
        discard;
    }

    return pshader_common_linear_depth_only(input);
}

