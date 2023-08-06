// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/common/gbuffer.hlsl"
#include "data:shaders/source/common/vertex.hlsl"
#include "data:shaders/source/common/normal.hlsl"
#include "data:shaders/source/common/lighting.hlsl"
#include "data:shaders/source/common/lighting_shading.hlsl"

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
};

struct geometry_poutput
{
    float4 accumulation : SV_Target0;
    float4 revealance : SV_Target1;
};

geometry_pinput vshader(vertex_input input)
{
    vertex v = load_vertex(input.vertex_id);
    geometry_instance_info vi = load_geometry_instance_info(input.instance_id);

    float4x4 mvp_matrix = mul(projection_matrix, mul(view_matrix, vi.model_matrix));

    geometry_pinput result;
    result.position = mul(mvp_matrix, float4(v.position, 1.0f));
    result.uv0 = v.uv0;
    result.world_normal = mul(vi.model_matrix, float4(v.normal, 0.0f));
    result.world_tangent = mul(vi.model_matrix, float4(v.tangent, 0.0f));
    result.world_position = mul(vi.model_matrix, float4(v.position, 1.0f));

    return result;
}

float calculate_weight(float z, float a)
{
    return clamp(pow(min(1.0, a * 10.0) + 0.01, 3.0) * 1e8 * 
                     pow(1.0f - z * 0.9, 3.0), 1e-2, 3e3);
}

geometry_poutput pshader(geometry_pinput input)
{
    float4 albedo = albedo_texture.Sample(albedo_sampler, input.uv0);;

    gbuffer_fragment f;
    f.albedo = albedo.rgb;
    f.flags = gbuffer_flag::none;
    f.metallic = metallic_texture.Sample(metallic_sampler, input.uv0).r;
    f.roughness = roughness_texture.Sample(roughness_sampler, input.uv0).r;
#if 0
    f.world_normal = normalize(input.world_normal).xyz,
#else
    f.world_normal = calculate_world_normal(
        unpack_compressed_normal(normal_texture.Sample(normal_sampler, input.uv0).xy),
        normalize(input.world_normal).xyz,
        normalize(input.world_tangent).xyz
    );
#endif
    f.world_position = input.world_position;

    float4 shaded_color = float4(shade_fragment(f).rgb, albedo.a * 0.25f);
    float weight = calculate_weight(input.position.z, shaded_color.a); 

    geometry_poutput output;
    output.accumulation = float4(shaded_color.rgb * shaded_color.a, shaded_color.a) * weight;
    output.revealance = shaded_color.a;
    return output;
}
