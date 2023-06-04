// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/common/gbuffer.hlsl"
#include "data:shaders/source/common/vertex.hlsl"
#include "data:shaders/source/common/normal.hlsl"

struct geometry_pinput
{
    float4 position : SV_POSITION;
    float2 uv0 : TEXCOORD0;
    float4 world_normal : TEXCOORD1;
    float4 world_bitangent : TEXCOORD2;
    float4 world_position : TEXCOORD3;
};

geometry_pinput vshader(vertex_input input)
{
    vertex v = load_vertex(input.vertex_id);
    geometry_instance_info vi = load_geometry_instance_info(input.instance_id);
 
    float4x4 mvp_matrix = mul(projection_matrix, mul(view_matrix, vi.model_matrix));

    geometry_pinput result;
    result.position = mul(mvp_matrix, float4(v.position, 1.0f));
    result.uv0 = v.uv0;
    result.world_normal = mul(vi.model_matrix, float4(v.normal, 1.0f));
    result.world_bitangent = mul(vi.model_matrix, float4(v.bitangent, 1.0f));
    result.world_position = mul(vi.model_matrix, float4(v.position, 1.0f));

    return result;
}

gbuffer_output pshader(geometry_pinput input)
{
    gbuffer_fragment f;
    f.albedo = albedo_texture.Sample(albedo_sampler, input.uv0).rgb;
    f.metallic = metallic_texture.Sample(metallic_sampler, input.uv0).r;
    f.roughness = roughness_texture.Sample(roughness_sampler, input.uv0).r;

    f.world_normal = calculate_world_normal(
        normal_texture.Sample(normal_sampler, input.uv0).xyz,
        normalize(input.world_normal).xyz,
        normalize(input.world_bitangent).xyz
    );   
    f.world_normal = input.world_normal.xyz;

    f.world_position = input.world_position.xyz;

    f.flags = 0;

    return encode_gbuffer(f);
}