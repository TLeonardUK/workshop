// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================

// Individual fragment of the gbuffer in decoded format.
// Use encode_gbuffer / decode_gbuffer to convert to input/output formats.
struct gbuffer_fragment
{
    float3 albedo;
    float metallic;
    float roughness;
    float3 world_normal;
    float3 world_position;
    uint flags;
};

// Output format when outputting to gbuffer.
struct gbuffer_output
{
    float4 buffer0 : SV_Target0;
    float4 buffer1 : SV_Target1;
    float4 buffer2 : SV_Target2;
};

// Encodes fragment into gbuffer_output format.
gbuffer_output encode_gbuffer(gbuffer_fragment fragment)
{
    gbuffer_output output;
    output.buffer0.xyz = fragment.albedo;
    output.buffer0.w = fragment.flags / 255.0f;
    output.buffer1.xyz = fragment.world_normal;
    output.buffer1.w = fragment.roughness;
    output.buffer2.xyz = fragment.world_position;
    output.buffer2.w = fragment.metallic;
    return output;
}

// Reads fragment from gbuffer at given uv.
gbuffer_fragment read_gbuffer(float2 uv)
{
    float4 buffer0 = gbuffer0_texture.Sample(gbuffer_sampler, uv);
    float4 buffer1 = gbuffer1_texture.Sample(gbuffer_sampler, uv);
    float4 buffer2 = gbuffer2_texture.Sample(gbuffer_sampler, uv);

    gbuffer_fragment result;
    result.albedo = buffer0.rgb;
    result.flags = uint(buffer0.a * 255.0f); 

    result.world_normal = buffer1.rgb;
    result.metallic = buffer1.a;

    result.world_position = buffer2.rgb;
    result.roughness = buffer2.a;

    return result;
}