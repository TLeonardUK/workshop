// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/fullscreen_pass.hlsl"
#include "data:shaders/source/common/gbuffer.hlsl"
#include "data:shaders/source/common/normal.hlsl"
#include "data:shaders/source/common/lighting.hlsl"

struct lightbuffer_output
{
    float4 color : SV_Target0;
};

enum light_type
{
    directional
};

light_state get_light_state(int index)
{
    instance_offset_info info = light_buffer.Load<instance_offset_info>(index * sizeof(instance_offset_info));
    light_state pb = table_byte_buffers[NonUniformResourceIndex(info.data_buffer_index)].Load<light_state>(info.data_buffer_offset);
    return pb;
}

lightbuffer_output pshader(fullscreen_pinput input)
{
    gbuffer_fragment frag = read_gbuffer(input.uv);

    float3 n = normalize(frag.world_normal);
    float3 v = normalize(view_world_position - frag.world_position);

    float3 metallic3 = float3(frag.metallic, frag.metallic, frag.metallic);

    float3 f0 = float3(0.04, 0.04, 0.04);
    f0 = lerp(f0, frag.albedo, metallic3);

    float3 lo = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < light_count; i++)
    {
        light_state light = get_light_state(i);

        float3 l = normalize(light.position - frag.world_position);
        float3 h = normalize(v + l);
        
        float3 radiance = 0.0f;
        if (light.type == light_type::directional)
        {
            // per-light radiance
            float distance = length(light.position - frag.world_position);
            float attenuation = 1.0f / (distance * distance);
            radiance = light.color * attenuation * 1000000;
        }

        // cook-torrance brdf
        float ndf = distribution_ggx(n, h, frag.roughness);
        float g = geometry_smith(n, v, l, frag.roughness);
        float3 f = fresnel_schlick(max(dot(h, v), 0.0), f0);

        float3 ks = f;
        float3 kd = float3(1.0, 1.0, 1.0) - ks;
        kd *= 1.0 - metallic3;

        float3 numerator = ndf * g * f;
        float denominator = 4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0) + 0.0001;
        float3 specular = numerator / denominator;

        float n_dot_l = max(dot(n, l), 0.0);
        lo += (kd * frag.albedo / pi + specular) * radiance * n_dot_l;
    }

    float3 ambient = float3(0.03, 0.03, 0.03) * frag.albedo;
    float3 color = ambient + lo;

    color = color / (color + float3(1.0, 1.0, 1.0));
    color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

    lightbuffer_output output;
    output.color = float4(color, 1.0f);

    return output;
}