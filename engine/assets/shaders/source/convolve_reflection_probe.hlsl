// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/fullscreen_pass.hlsl"
#include "data:shaders/source/common/math.hlsl"
#include "data:shaders/source/common/lighting.hlsl"

struct pshader_output
{
    float4 color : SV_Target0;
};

//    float3 result = get_cubemap_normal(source_texture_face, source_texture_size, uint2(input.uv0 * source_texture_size) + uint2(-1, -1));


float4 convolve(fullscreen_pinput input)
{
    float3 n = get_cubemap_normal(source_texture_face, source_texture_size, uint2(input.uv0 * source_texture_size) + uint2(-1, -1));
    float3 r = n;
    float3 v = r;

    float3 accumulated_color = 0.0f;
    float accumulated_weight = 0.0f;

    const uint SAMPLE_COUNT = 1024;
    for (uint i = 0; i < SAMPLE_COUNT; i++)
    {
        float2 xi = hammersley(i, SAMPLE_COUNT);
        float3 h  = importance_sample_ggx(xi, n, roughness);
        float3 l  = normalize(2.0 * dot(v, h) * h - v);
        
        float n_dot_l = max(dot(n, l), 0.0);
        if (n_dot_l > 0.0)
        {
            float d = distribution_ggx(n, h, roughness);
            float n_dot_h = max(dot(n, h), 0.0);
            float h_dot_v = max(dot(h, v), 0.0);
            float pdf = d * n_dot_h / (4.0 * h_dot_v) + 0.0001;

            float resolution = source_texture_size.x;
            float sa_texel = 4.0 * pi / (6.0 * resolution * resolution);
            float sa_sample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

            float mip_level = (roughness == 0.0 ? 0.0 : 0.5 * log2(sa_sample / sa_texel)); 
        
            accumulated_color += source_texture.SampleLevel(source_texture_sampler, l, mip_level).rgb * n_dot_l;
            accumulated_weight += n_dot_l;
        }
    }

    accumulated_color /= accumulated_weight;
    return float4(accumulated_color, 1.0f);
}

pshader_output pshader(fullscreen_pinput input)
{
    pshader_output output;
    output.color = convolve(input);
    return output;
}