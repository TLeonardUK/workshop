// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/fullscreen_pass.hlsl"
#include "data:shaders/source/common/math.hlsl"

struct pshader_output
{
    float4 color : SV_Target0;
};

pshader_output pshader(fullscreen_pinput input)
{
    float3 a = get_cubemap_normal(source_texture_face, source_texture_size, uint2(input.uv * source_texture_size) + uint2(-1, -1));
    float3 b = get_cubemap_normal(source_texture_face, source_texture_size, uint2(input.uv * source_texture_size) + uint2(+1, -1));
    float3 c = get_cubemap_normal(source_texture_face, source_texture_size, uint2(input.uv * source_texture_size) + uint2(0, 0));
    float3 d = get_cubemap_normal(source_texture_face, source_texture_size, uint2(input.uv * source_texture_size) + uint2(-1, +1));
    float3 e = get_cubemap_normal(source_texture_face, source_texture_size, uint2(input.uv * source_texture_size) + uint2(+1, +1));
    float4 result = 
        (source_texture.Sample(source_texture_sampler, a) +
         source_texture.Sample(source_texture_sampler, b) +
         source_texture.Sample(source_texture_sampler, c) +
         source_texture.Sample(source_texture_sampler, d) +
         source_texture.Sample(source_texture_sampler, e)) / 5.0f;

    pshader_output output;
    output.color = result;
    return output;
}