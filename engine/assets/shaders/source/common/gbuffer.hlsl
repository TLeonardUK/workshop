// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _GBUFFER_HLSL_
#define _GBUFFER_HLSL_

// Individual fragment of the gbuffer in decoded format.
// Use encode_gbuffer / decode_gbuffer to convert to input/output formats.
struct gbuffer_fragment
{
    float3 albedo;
    float opacity;
    float metallic;
    float3 world_normal;
    float roughness;
    float3 world_position;
    float4 debug_data;
};

// Output format when outputting to gbuffer.
struct gbuffer_output
{
    float4 buffer0 : SV_Target0;
    float4 buffer1 : SV_Target1;
    float4 buffer2 : SV_Target2;
    float4 buffer3 : SV_Target3;
};

// Encodes fragment into gbuffer_output format.
gbuffer_output encode_gbuffer(gbuffer_fragment fragment)
{
    gbuffer_output output;
    output.buffer0.xyz = fragment.albedo;
    output.buffer0.w = fragment.opacity;

    output.buffer1.xyz = fragment.world_normal;
    output.buffer1.w = fragment.roughness;

    output.buffer2.xyz = fragment.world_position;
    output.buffer2.w = fragment.metallic;
    
    output.buffer3.xyzw = fragment.debug_data;
    
    return output;
}

// Reads fragment from gbuffer at given uv.
gbuffer_fragment read_gbuffer(float2 uv)
{
    float4 buffer0 = gbuffer0_texture.Sample(gbuffer_sampler, uv);
    float4 buffer1 = gbuffer1_texture.Sample(gbuffer_sampler, uv);
    float4 buffer2 = gbuffer2_texture.Sample(gbuffer_sampler, uv);
    float4 buffer3 = gbuffer3_texture.Sample(gbuffer_sampler, uv);

    gbuffer_fragment result;
    result.albedo = buffer0.xyz;
    result.opacity = buffer0.w;

    result.world_normal = buffer1.xyz;
    result.roughness = buffer1.w;

    result.world_position = buffer2.xyz;
    result.metallic = buffer2.w;

    result.debug_data = buffer3.xyzw;

    return result;
}

#endif