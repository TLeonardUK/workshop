// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/fullscreen_pass.hlsl"
#include "data:shaders/source/common/gbuffer.hlsl"
#include "data:shaders/source/common/normal.hlsl"
#include "data:shaders/source/common/lighting.hlsl"

// ================================================================================================
//  Types
// ================================================================================================

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
    return table_byte_buffers[NonUniformResourceIndex(info.data_buffer_index)].Load<light_state>(info.data_buffer_offset);
}

shadow_map_state get_shadow_map_state(int index)
{
    instance_offset_info info = shadow_map_buffer.Load<instance_offset_info>(index * sizeof(instance_offset_info));
    return table_byte_buffers[NonUniformResourceIndex(info.data_buffer_index)].Load<shadow_map_state>(info.data_buffer_offset);
}

// ================================================================================================
//  Shadow map calculation
// ================================================================================================
 
struct cascade_info
{
    float blend;
    int primary_cascade_index;
    int secondary_cascade_index;
};

// Takes the position of a fragment and a light and determines the best cascade to use.
// If in the blending area of two cascades also returns the blend factor and the other cascade to use.
cascade_info get_shadow_cascade_info(gbuffer_fragment frag, light_state light)
{
    cascade_info info;
    info.blend = 1.0;
    info.primary_cascade_index = light.shadow_map_count - 1;
    info.secondary_cascade_index = info.primary_cascade_index + 1;

    for (int i = 0; i < light.shadow_map_count; i++)
    {
        shadow_map_state state = get_shadow_map_state(light.shadow_map_start_index + i);
        float4 world_position_light_space = mul(state.shadow_matrix, float4(frag.world_position, 1.0));
        float4 world_position_light_clip_space = world_position_light_space / world_position_light_space.w;

        if (world_position_light_clip_space.z > -1.0 && world_position_light_clip_space.z < 1.0)
        {
            float2 shadow_map_coord = world_position_light_clip_space.xy * 0.5 + 0.5;
            
            // TODO: Fix shadow maps being upside down :squint:
            shadow_map_coord = float2(shadow_map_coord.x, 1.0f - shadow_map_coord.y);

            if (shadow_map_coord.x >= 0.0 && shadow_map_coord.y >= 0.0 && shadow_map_coord.x <= 1.0 && shadow_map_coord.y <= 1.0)
            {
				float min_distance = 
                    min(
                        min(
                            min(shadow_map_coord.x, 1.0 - shadow_map_coord.x),
					        shadow_map_coord.y
                        ),
					    1.0 - shadow_map_coord.y
                    );
                
                info.blend = 1.0 - clamp(min_distance / light.cascade_blend_factor, 0.0, 1.0);
                info.primary_cascade_index = i;
                info.secondary_cascade_index = i + 1;

                break;
            }
        }
    }

    return info;
}

float sample_shadow_map(gbuffer_fragment frag, light_state light, int cascade_index)
{
    // If outside cascade count, return no shadow.
    if (cascade_index >= light.shadow_map_count)
    {
        return 1.0;
    }

    shadow_map_state state = get_shadow_map_state(light.shadow_map_start_index + cascade_index);
    float4 world_position_light_space = mul(state.shadow_matrix, float4(frag.world_position, 1.0));
    float4 world_position_light_clip_space = world_position_light_space / world_position_light_space.w;

    // If outside shadow map, return no shadow.
    if (world_position_light_clip_space.z <= -1.0 || world_position_light_clip_space.z >= 1.0)
    {
        return 1.0;
    }

    float2 shadow_map_coord = world_position_light_clip_space.xy * 0.5 + 0.5;

    // TODO: Fix shadow maps being upside down :squint:
    shadow_map_coord = float2(shadow_map_coord.x, 1.0f - shadow_map_coord.y);

    // TODO: Do some fancier sampling here.
    float depth = table_texture_2d[NonUniformResourceIndex(state.depth_map_index)].Sample(shadow_map_sampler, shadow_map_coord).r;
    float bias = 0.001f;

    return depth < world_position_light_clip_space.z - bias ? 0.0 : 1.0;
}

float calculate_shadow_directional(gbuffer_fragment frag, light_state light)
{
    // If normal is in direction of the light, we know we are in shadow as the light
    // is hitting our back.
    if (dot(frag.world_normal, light.direction) > 0.0f)
    {
        return 0.0f;
    }

    cascade_info cascade_info = get_shadow_cascade_info(frag, light);

    float primary_cascade_value = sample_shadow_map(frag, light, cascade_info.primary_cascade_index);
    float secondary_cascade_value = sample_shadow_map(frag, light, cascade_info.secondary_cascade_index);

    return lerp(primary_cascade_value, secondary_cascade_value, cascade_info.blend);
}

float calculate_shadow_point(gbuffer_fragment frag, light_state light)
{
    return 1.0f;
}

float calculate_shadow_spotlight(gbuffer_fragment frag, light_state light)
{
    return 1.0f;
}

// ================================================================================================
//  Attenuation calculation
// ================================================================================================

float calculate_attenuation_directional(gbuffer_fragment frag, light_state light)
{
    return 1.0f;
}

float calculate_attenuation_point(gbuffer_fragment frag, light_state light)
{
    float distanceSqr = square(length(light.position - frag.world_position));
    float light_range_inv = 1.0f / square(light.range);
    float range_attenuation = square(saturate(1.0 - square(distanceSqr * light_range_inv)));
    float attenuation = range_attenuation / distanceSqr;

    return attenuation;
}

float calculate_attenuation_spotlight(gbuffer_fragment frag, light_state light)
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

// ================================================================================================
//  Lighting calculation
// ================================================================================================

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
    float shadow_attenuation = 1.0f;
    if (light.type == light_type::directional)
    {
        attenuation = calculate_attenuation_directional(frag, light);
        shadow_attenuation = calculate_shadow_directional(frag, light);
    }
    else if (light.type == light_type::point_)
    {
        attenuation = calculate_attenuation_point(frag, light);
        shadow_attenuation = calculate_attenuation_point(frag, light);
    }
    else if (light.type == light_type::spotlight)
    {
        attenuation = calculate_attenuation_spotlight(frag, light);
        shadow_attenuation = calculate_attenuation_spotlight(frag, light);
    }

    // Calculate fresnel reflection
    float3 radiance = light.color * light.intensity * attenuation * shadow_attenuation;
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
    return float3(0.03, 0.03, 0.03) * frag.albedo;
}

// ================================================================================================
//  Entry points
// ================================================================================================

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