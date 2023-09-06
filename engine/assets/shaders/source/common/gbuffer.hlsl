// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _GBUFFER_HLSL_
#define _GBUFFER_HLSL_

// If defined this adds an extra plane to the gbuffer for writing debug data to.
// This needs to be paired with the one in renderer.h
//#define GBUFFER_DEBUG_DATA 

// Flags describing the state of individual fragments in the gbuffer.
// This should be kept in sync with render_gpu_flags in cpp.
enum render_gpu_flags
{
    none        = 0,

    // No lighting calculations will be performed on fragment, it will 
    // be rendered full bright.
    unlit       = 1,

    // Object is selected and will have an outline drawn around it.
    selected    = 2
};

// Individual fragment of the gbuffer in decoded format.
// Use encode_gbuffer / decode_gbuffer to convert to input/output formats.
struct gbuffer_fragment
{
    float3 albedo;
    float metallic;
    float3 world_normal;
    float roughness;
    float3 world_position;
    uint flags;
    
    float2 uv;

#ifdef GBUFFER_DEBUG_DATA
    float4 debug_data;
#endif
};

// Output format when outputting to gbuffer.
struct gbuffer_output
{
    float4 buffer0 : SV_Target0;
    float4 buffer1 : SV_Target1;
    float4 buffer2 : SV_Target2;
#ifdef GBUFFER_DEBUG_DATA
    float4 buffer3 : SV_Target3;
#endif
};

// Gbuffer output with depth
struct gbuffer_output_with_depth
{
    float4 buffer0 : SV_Target0;
    float4 buffer1 : SV_Target1;
    float4 buffer2 : SV_Target2;
    float depth : SV_Depth;
};

// Encodes fragment into gbuffer_output format.
gbuffer_output encode_gbuffer(gbuffer_fragment fragment)
{
    gbuffer_output output;
    output.buffer0.xyz = fragment.albedo;
    output.buffer0.w = (fragment.flags / 255.0f);

    output.buffer1.xyz = fragment.world_normal;
    output.buffer1.w = fragment.roughness;

    output.buffer2.xyz = fragment.world_position;
    output.buffer2.w = fragment.metallic;

#ifdef GBUFFER_DEBUG_DATA    
    output.buffer3.xyzw = fragment.debug_data;
#endif

    return output;
}

// Reads fragment from gbuffer at given uv.
gbuffer_fragment read_gbuffer(float2 uv)
{
    float4 buffer0 = gbuffer0_texture.Sample(gbuffer_sampler, uv);
    float4 buffer1 = gbuffer1_texture.Sample(gbuffer_sampler, uv);
    float4 buffer2 = gbuffer2_texture.Sample(gbuffer_sampler, uv);
#ifdef GBUFFER_DEBUG_DATA    
    float4 buffer3 = gbuffer3_texture.Sample(gbuffer_sampler, uv);
#endif

    gbuffer_fragment result;
    result.albedo = buffer0.xyz;
    result.flags = (uint)round(buffer0.w * 255.0f);

    result.world_normal = buffer1.xyz;
    result.roughness = buffer1.w;

    result.world_position = buffer2.xyz;
    result.metallic = buffer2.w;

#ifdef GBUFFER_DEBUG_DATA
    result.debug_data = buffer3.xyzw;
#endif

    result.uv = uv;

    return result;
}

// Reads fragment from gbuffer at given uv but without interpolation
gbuffer_fragment load_gbuffer(float2 uv)
{
    uint3 coords = uint3(uv * gbuffer_dimensions, 0);

    float4 buffer0 = gbuffer0_texture.Load(coords);
    float4 buffer1 = gbuffer1_texture.Load(coords);
    float4 buffer2 = gbuffer2_texture.Load(coords);
#ifdef GBUFFER_DEBUG_DATA    
    float4 buffer3 = gbuffer3_texture.Load(coords);
#endif

    gbuffer_fragment result;
    result.albedo = buffer0.xyz;
    result.flags = (uint)round(buffer0.w * 255.0f);

    result.world_normal = buffer1.xyz;
    result.roughness = buffer1.w;

    result.world_position = buffer2.xyz;
    result.metallic = buffer2.w;

#ifdef GBUFFER_DEBUG_DATA
    result.debug_data = buffer3.xyzw;
#endif

    result.uv = uv;

    return result;
}

#endif