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
    directional,
    point_,
    spotlight
};

light_state get_light_state(int index)
{
    instance_offset_info info = light_buffer.Load<instance_offset_info>(index * sizeof(instance_offset_info));
    light_state pb = table_byte_buffers[NonUniformResourceIndex(info.data_buffer_index)].Load<light_state>(info.data_buffer_offset);
    return pb;
}

float3 calculate_attenuation_directional(gbuffer_fragment frag, light_state light)
{
    return 1.0f;
}

float3 calculate_attenuation_point(gbuffer_fragment frag, light_state light)
{
    float distanceSqr = square(length(light.position - frag.world_position));
    float light_range_inv = 1.0f / square(light.range);
    float range_attenuation = square(saturate(1.0 - square(distanceSqr * light_range_inv)));
    float attenuation = range_attenuation / distanceSqr;

    return attenuation;
}

float3 calculate_attenuation_spotlight(gbuffer_fragment frag, light_state light)
{
    float3 light_to_pixel = normalize(frag.world_position - light.position);
    float spot = dot(light_to_pixel, light.direction);

    float inner_dot = 1.0f - (light.inner_radius / half_pi);
    float outer_dot = 1.0f - (light.outer_radius / half_pi);

    if (spot > outer_dot)
    {
        float range = inner_dot - outer_dot;
        float delta = saturate((spot - inner_dot) / range);

        return calculate_attenuation_point(frag, light) * delta;
    }
    else
    {
        return 0.0;
    }
}

float3 calculate_direct_lighting(gbuffer_fragment frag, light_state light)
{
    float3 normal = normalize(frag.world_normal);
    float3 view_direction = normalize(view_world_position - frag.world_position);
    float3 metallic = frag.metallic;

    // Direction between light and frag.    
    float3 light_frag_direction = 0.0f;
    if (light.type == light_type::directional)
    {
        light_frag_direction = -light.direction;
    }
    else
    {
        light_frag_direction = normalize(light.position - frag.world_position);
    }

    // Half vector between view_direction and light_direction.
    float3 view_light_half = normalize(view_direction + light_frag_direction);
    
    // Calculate radiance based on the light type.
    float attenuation = 1.0f;
    if (light.type == light_type::directional)
    {
        attenuation = calculate_attenuation_directional(frag, light);
    }
    else if (light.type == light_type::point_)
    {
        attenuation = calculate_attenuation_point(frag, light);
    }
    else if (light.type == light_type::spotlight)
    {
        attenuation = calculate_attenuation_spotlight(frag, light);
    }

    // Calculate fresnel reflection
    float3 radiance = light.color * light.intensity * attenuation;
    float3 fresnel_reflectance = lerp(dielectric_fresnel, frag.albedo, metallic);

    // Calculate fresnel term.
    float3 f = fresnel_schlick(max(dot(view_light_half, view_direction), 0.0), fresnel_reflectance);
    // Calculate normal distribution for specular brdf
    float d = distribution_ggx(normal, view_light_half, frag.roughness);
    // Calculate geometric attenuation for specular brdf.
    float g = geometry_smith(normal, view_direction, light_frag_direction, frag.roughness);

    // Diffuse scattering based on metallic.
    float3 kd = lerp(float3(1.0, 1.0, 1.0) - f, float3(0.0, 0.0, 0.0), metallic);

    // Lambert diffuse brdf.
    float3 diffuse_brdf = (kd * frag.albedo) / pi;

    // Cook-Torrance brdf
    float3 numerator = d * g * f;
    float denominator = 4.0 * max(dot(normal, view_direction), 0.0) * max(dot(normal, light_frag_direction), 0.0) + 0.0001;
    float3 specular_brdf = numerator / denominator;

    // Total contribution for this light.
    float n_dot_l = max(dot(normal, light_frag_direction), 0.0);
    return (diffuse_brdf + specular_brdf) * radiance * n_dot_l;
}

float3 calculate_ambient_lighting(gbuffer_fragment frag)
{
    // TODO: Do IBL.
//    return float3(0.0f, 0.0f, 0.0f);
    return float3(0.03, 0.03, 0.03) * frag.albedo;
}

lightbuffer_output pshader(fullscreen_pinput input)
{
    gbuffer_fragment frag = read_gbuffer(input.uv);

    // Calculate ambient lighting from our light probes.
    float3 ambient_lighting = calculate_ambient_lighting(frag);
    
    // Calculate direct contribution of all lights that effect us.
    float3 direct_lighting = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < light_count; i++)
    {
        light_state light = get_light_state(i);
        direct_lighting += calculate_direct_lighting(frag, light);
    }

    lightbuffer_output output;
    output.color = float4(direct_lighting + ambient_lighting, 1.0f);
    return output;
}