// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _LIGHTING_SHADING_HLSL_
#define _LIGHTING_SHADING_HLSL_

#include "data:shaders/source/common/gbuffer.hlsl"
#include "data:shaders/source/common/normal.hlsl"
#include "data:shaders/source/common/math.hlsl"
#include "data:shaders/source/common/lighting.hlsl"
#include "data:shaders/source/common/light_probe_grid.hlsl"
#include "data:shaders/source/common/reflection_probe.hlsl"
#include "data:shaders/source/common/consts.hlsl"

// ================================================================================================
//  Utilities to get the various states from the indirection buffers.
// ================================================================================================
 
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

enum shadow_filter
{
    // Blocky, woo.
    no_filter,
    // Percentage closer filter.
    pcf,
    // PCF using a rotated poisson disc for tap locations.
    pcf_poisson
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

            // Invert y axis as directx's uv's are inverted.            
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

float sample_shadow_map(gbuffer_fragment frag, light_state light, int cascade_index, shadow_filter filter, int samples)
{
    // If outside cascade count, return no shadow.
    if (cascade_index < 0 || cascade_index >= light.shadow_map_count)
    {
        return 1.0;
    }

    shadow_map_state state = get_shadow_map_state(light.shadow_map_start_index + cascade_index);
    float4 world_position_light_space = mul(state.shadow_matrix, float4(frag.world_position, 1.0));
    float4 world_position_light_clip_space = world_position_light_space / world_position_light_space.w;

    float n_dot_l = dot(light.position, frag.world_position);
    float2 texel_size = float2(1.0 / state.depth_map_size, 1.0 / state.depth_map_size);

    // If outside shadow map, return no shadow.
    if (world_position_light_clip_space.z <= -1.0 || world_position_light_clip_space.z >= 1.0)
    {
        return 1.0;
    }

    float2 shadow_map_coord = world_position_light_clip_space.xy * 0.5 + 0.5;

    // Invert y axis as directx's uv's are inverted.
    shadow_map_coord = float2(shadow_map_coord.x, 1.0f - shadow_map_coord.y);

    // Sloped bias to filter out acne on slanted surfaces.
    float bias = clamp(0.005f * tan(acos(saturate(n_dot_l))), 0.001f, 0.001f); 
    float result = 0.0f;
    
    // Filter if required.
    Texture2D shadow_map =  table_texture_2d[NonUniformResourceIndex(state.depth_map_index)];
    switch (filter)
    {
        default:
        case shadow_filter::no_filter:
        {
            float depth = shadow_map.SampleLevel(shadow_map_sampler, shadow_map_coord, 0).r;

            result = (depth < world_position_light_clip_space.z - bias ? 0.0 : 1.0);
            break;
        }
        case shadow_filter::pcf:
        {
            int direction_samples = sqrt(samples) / 2;
            int sample_count = 0;

            for (int y = -direction_samples; y <= direction_samples; y++)
            {
                for (int x = -direction_samples; x <= direction_samples; x++)
                {
                    float2 uv = shadow_map_coord + float2(x, y) * texel_size;      
                    float depth = shadow_map.SampleLevel(shadow_map_sampler, uv, 0).r;
                    result += (depth < world_position_light_clip_space.z - bias ? 0.0 : 1.0);
                    sample_count++;
                }
            }

            result /= sample_count;
            break;
        }
        case shadow_filter::pcf_poisson:
        {
            // TODO: For future consideration, use a variable penumbra
            // https://andrew-pham.blog/2019/08/03/percentage-closer-soft-shadows/
            // https://developer.download.nvidia.com/whitepapers/2008/PCSS_Integration.pdf

            float kernel_size = 2.0f;
            
            for (int i = 0; i < samples; i++)
            {
                int disk_index = i % 16;
                float angle = random(float4(frag.world_position.xyz, i));
                float2 rotation = float2(
                    cos(angle),
                    sin(angle)
                );		
                float2 poisson_offset = float2(
                    rotation.x * poisson_disk[disk_index].x - rotation.y * poisson_disk[disk_index].y,
                    rotation.y * poisson_disk[disk_index].x + rotation.x * poisson_disk[disk_index].y
                );
       
                float2 uv = shadow_map_coord + poisson_offset * texel_size * kernel_size;
                float depth = shadow_map.SampleLevel(shadow_map_sampler, uv, 0).r;
                result += (depth < world_position_light_clip_space.z - bias ? 0.0 : 1.0);
            }

            result /= samples;
            break;
        }
    }
    
    return result;
}

float sample_shadow_cubemap(gbuffer_fragment frag, light_state light, int cascade_index, shadow_filter filter, int samples)
{
    // If outside cascade count, return no shadow.
    if (cascade_index < 0 || cascade_index >= light.shadow_map_count)
    {
        return 1.0;
    }

    shadow_map_state state = get_shadow_map_state(light.shadow_map_start_index + cascade_index);
    float n_dot_l = dot(light.position, frag.world_position);

    // If outside shadow map, return no shadow.
    float current_depth = length(frag.world_position.xyz - light.position.xyz);        
    if (current_depth < 0 || current_depth > state.z_far)
    {
        return 1.0;
    }

    // Sloped bias to filter out acne on slanted surfaces.
    float bias = state.z_near;
    float result = 0.0f;

    TextureCube shadow_map_cube = table_texture_cube[NonUniformResourceIndex(state.depth_map_index)];
    switch (filter)
    {
        // TODO: Calculate sampled UV's so we can do "normal" filtering.
        //       The filtering below is temporary at best.
        default:
        case shadow_filter::no_filter:
        {
            float3 frag_to_light = frag.world_position.xyz - light.position.xyz;
            float current_depth = length(frag_to_light);        
            float depth = shadow_map_cube.SampleLevel(shadow_map_sampler, frag_to_light, 0).r * state.z_far;

            result += (depth < current_depth - bias ? 0.0 : 1.0);

            break;
        }
        case shadow_filter::pcf: 
        {
            float kernel_size = 1.0f * (1024.0f / state.depth_map_size);

            int direction_samples = sqrt(samples) / 2;
            int sample_count = 0;

            for (int y = -direction_samples; y <= direction_samples; y++)
            {
                for (int x = -direction_samples; x <= direction_samples; x++)
                {
                    float3 normal = normalize(frag.world_position.xyz - light.position.xyz);
                    float3 tangent = make_perpendicular(normal);
                    float3 bitangent = cross(normal, tangent);

                    float3 offset = ((tangent * x) + (bitangent * y)) * kernel_size;
                    float3 frag_to_light = (frag.world_position.xyz + offset) - light.position.xyz;
                    float current_depth = length(frag_to_light);        
        
                    float depth = shadow_map_cube.SampleLevel(shadow_map_sampler, frag_to_light, 0).r * state.z_far;
                    result += (depth < current_depth - bias ? 0.0 : 1.0);
                    sample_count++;
                }
            }

            result /= sample_count;
            break;
        }
        case shadow_filter::pcf_poisson:
        {
            float kernel_size = 2.0f * (1024.0f / state.depth_map_size);
            
            for (int i = 0; i < samples; i++)
            {
                int disk_index = i % 16;
                float angle = random(float4(frag.world_position.xyz, i));
                float2 rotation = float2(
                    cos(angle),
                    sin(angle)
                );		
                float2 poisson_offset = float2(
                    rotation.x * poisson_disk[disk_index].x - rotation.y * poisson_disk[disk_index].y,
                    rotation.y * poisson_disk[disk_index].x + rotation.x * poisson_disk[disk_index].y
                );

                float3 normal = normalize(frag.world_position.xyz - light.position.xyz);
                float3 tangent = make_perpendicular(normal);
                float3 bitangent = cross(normal, tangent);
       
                float3 offset = ((tangent * poisson_offset.x) + (bitangent * poisson_offset.y)) * kernel_size;
                float3 frag_to_light = (frag.world_position.xyz + offset) - light.position.xyz;
                float current_depth = length(frag_to_light);        
       
                float depth = shadow_map_cube.SampleLevel(shadow_map_sampler, frag_to_light, 0).r * state.z_far;
                result += (depth < current_depth - bias ? 0.0 : 1.0);
            }

            result /= samples;
            break;
        }
    }

    return result;
}

float2 calculate_shadow_directional(gbuffer_fragment frag, light_state light)
{
    cascade_info cascade_info = get_shadow_cascade_info(frag, light);

    // If normal is in direction of the light, we know we are in shadow as the light
    // is hitting our back.
    if (dot(frag.world_normal, light.direction) > 0.0f)
    {
        return float2(0.0f, cascade_info.primary_cascade_index);
    }

    float primary_cascade_value = sample_shadow_map(frag, light, cascade_info.primary_cascade_index, shadow_filter::pcf_poisson, 16);
    float secondary_cascade_value = sample_shadow_map(frag, light, cascade_info.secondary_cascade_index, shadow_filter::pcf_poisson, 16);

    return float2(
        lerp(primary_cascade_value, secondary_cascade_value, cascade_info.blend),
        cascade_info.primary_cascade_index
    );
}

float2 calculate_shadow_point(gbuffer_fragment frag, light_state light)
{
    float cascade_value = sample_shadow_cubemap(frag, light, 0, shadow_filter::pcf_poisson, 16);

    return float2(
        cascade_value,
        0.0f
    );
}

float2 calculate_shadow_spotlight(gbuffer_fragment frag, light_state light)
{
    // If normal is in direction of the light, we know we are in shadow as the light
    // is hitting our back.
    if (dot(frag.world_normal, light.direction) > 0.0f)
    {
        return float2(0.0f, 0.0f);
    }

    float cascade_value = sample_shadow_map(frag, light, 0, shadow_filter::pcf_poisson, 16);

    return float2(
        cascade_value,
        0.0f
    );
}

// ================================================================================================
//  Attenuation calculation
// ================================================================================================

float calculate_attenuation_importance(gbuffer_fragment frag, light_state light)
{
    float distance = length(view_world_position - light.position);
    float importance_fade = 1.0f;
    float start_fade_distance =  light.importance_distance * LIGHT_IMPORTANCE_FADE_DISTANCE;
    float end_fade_distance =  light.importance_distance;
    if (distance > start_fade_distance)
    {
        importance_fade = 1.0f - saturate((distance - start_fade_distance) / (end_fade_distance - start_fade_distance));
    } 

    return importance_fade;
}

float calculate_attenuation_directional(gbuffer_fragment frag, light_state light)
{
    return calculate_attenuation_importance(frag, light);
}

float calculate_attenuation_point(gbuffer_fragment frag, light_state light)
{
    float importance_fade = calculate_attenuation_importance(frag, light);

    float frag_to_light_distance = length(light.position - frag.world_position);

#ifdef USE_INVERSE_SQUARE_LAW
    float range_squared = square(light.range * light.range);
    float distanced_squared = square(frag_to_light_distance);
    float inv_range = 1.0 / max(range_squared, 0.00001f);
    float range_attenuation = square(saturate(1.0 - square(distanced_squared * inv_range)));
    float attenuation = range_attenuation / distanced_squared;
    return saturate(attenuation) * importance_fade;
#else
    return saturate(1.0f - (frag_to_light_distance / light.range)) * importance_fade;
#endif
}

float calculate_attenuation_spotlight(gbuffer_fragment frag, light_state light)
{
    float3 light_to_pixel = normalize(frag.world_position - light.position);
    float spot = dot(light_to_pixel, light.direction);

    float inner_radius = light.inner_radius;
    float outer_radius = light.outer_radius;

    float inner_dot = 1.0f - saturate(inner_radius / pi);
    float outer_dot = 1.0f - saturate(outer_radius / pi);

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

struct direct_lighting_result
{
    float3 lighting;
    float2 shadow_attenuation_and_cascade_index;
};

direct_lighting_result calculate_direct_lighting(gbuffer_fragment frag, light_state light, bool is_transparent_or_ray)
{    
    float3 normal = normalize(frag.world_normal);
    float3 view_direction = normalize(view_world_position - frag.world_position);
    float3 metallic = frag.metallic;

    float3 albedo = frag.albedo;
#ifndef DISABLE_VISUALIZATION_MODES
    if (visualization_mode == visualization_mode_t::lighting)
    {
        albedo = float3(1.0f, 1.0f, 1.0f);
    }
#endif

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
    float2 shadow_attenuation_and_cascade_index = 1.0f;
    
    if (light.type == light_type::directional)
    {
        attenuation = calculate_attenuation_directional(frag, light);
        shadow_attenuation_and_cascade_index = calculate_shadow_directional(frag, light);
    }
    else if (light.type == light_type::point_)
    {
        attenuation = calculate_attenuation_point(frag, light);
        shadow_attenuation_and_cascade_index = calculate_shadow_point(frag, light);
    }
    else if (light.type == light_type::spotlight)
    {
        attenuation = calculate_attenuation_spotlight(frag, light);
        shadow_attenuation_and_cascade_index = calculate_shadow_spotlight(frag, light);
    }
    
    // Calculate fresnel reflection
    float3 radiance = light.color * light.intensity * attenuation * shadow_attenuation_and_cascade_index.r;
    float3 fresnel_reflectance = lerp(dielectric_fresnel, albedo, metallic);

    // Calculate fresnel term.
    float3 f = fresnel_schlick(max(dot(view_light_half, view_direction), 0.0), fresnel_reflectance);
    // Calculate normal distribution for specular brdf
    float d = distribution_ggx(normal, view_light_half, frag.roughness);
    // Calculate geometric attenuation for specular brdf.
    float g = geometry_smith(normal, view_direction, light_frag_direction, frag.roughness);

    // Diffuse scattering based on metallic.
    float3 kd = lerp(float3(1.0, 1.0, 1.0) - f, float3(0.0, 0.0, 0.0), metallic);

    // Lambert diffuse brdf.
    float3 diffuse_brdf = (kd * albedo) / pi;

    // Cook-Torrance brdf
    float3 numerator = d * g * f;
    float denominator = 4.0 * max(dot(normal, view_direction), 0.0) * max(dot(normal, light_frag_direction), 0.0) + 0.0001;
    float3 specular_brdf = numerator / denominator;

    // Total contribution for this light.
    float ao = 1.0f;
    if (!is_transparent_or_ray)
    {
        ao = ao_texture.SampleLevel(ao_sampler, frag.uv * ao_uv_scale, 0).r;
    }
    float ao_multiplier = lerp(1.0f, ao, ao_direct_light_effect);

    float n_dot_l = max(dot(normal, light_frag_direction), 0.0);
    float3 lighting = (diffuse_brdf + specular_brdf) * radiance * n_dot_l * ao_multiplier;

    // Bundle up relevant values and return.
    direct_lighting_result ret;
    ret.lighting = lighting;
    ret.shadow_attenuation_and_cascade_index = shadow_attenuation_and_cascade_index;
    return ret;
}

float3 calculate_ambient_lighting(gbuffer_fragment frag, bool is_transparent_or_ray)
{    
#ifdef DISABLE_AMBIENT_LIGHTING    
    return float3(0.0f, 0.0f, 0.0f);
#else
    if (use_constant_ambient)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    if (!apply_ambient_lighting)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    float3 normal = normalize(frag.world_normal);
    float3 view_direction = normalize(view_world_position - frag.world_position);
    float3 reflect_direction = reflect(-view_direction, normal);   

    float3 metallic = frag.metallic;
    float3 albedo = frag.albedo;

#ifndef DISABLE_VISUALIZATION_MODES
    if (visualization_mode == visualization_mode_t::lighting)
    {
        albedo = float3(1.0f, 1.0f, 1.0f);
    }
#endif

    float3 fresnel_reflectance = lerp(dielectric_fresnel, albedo, metallic);
    float3 F = fresnel_schlick_roughness(max(dot(normal, view_direction), 0.0), fresnel_reflectance, frag.roughness);;
    float3 kS = F;
    float3 kD = 1.0f - kS;
    kD *= 1.0f - metallic;

    // Sample light grids for our diffuse term.
    float3 irradiance = sample_light_probe_grids(light_probe_grid_count, light_probe_grid_buffer, frag.world_position, frag.world_normal);
    float3 diffuse = irradiance * albedo / pi;

    // Sample our reflection probes + BRDF LUT to calculate our specular term.
    float3 reflection_probe_color = sample_reflection_probes(frag.world_position, reflect_direction, reflection_probe_count, reflection_probe_buffer, frag.roughness);
    float2 brdf = brdf_lut.SampleLevel(brdf_lut_sampler, float2(max(dot(normal, view_direction), 0.0), frag.roughness), 0).rg;
    float3 specular = reflection_probe_color * (F * brdf.x + brdf.y);

    // Grab the AO output to tint our ambient term.
    float ao = 1.0f;
    if (!is_transparent_or_ray)
    {
        ao = ao_texture.SampleLevel(ao_sampler, frag.uv * ao_uv_scale, 0).r;
    }

    float3 ambient = (kD * diffuse + specular) * ao;

#ifndef DISABLE_VISUALIZATION_MODES
    if (visualization_mode == visualization_mode_t::indirect_specular)
    {
        ambient = specular;
    }
    else if (visualization_mode == visualization_mode_t::indirect_diffuse)
    {
        ambient = kD * diffuse;
    }
    else if (visualization_mode == visualization_mode_t::ao)
    {
        ambient = ao;
    }
#endif

    return ambient;
#endif
}

uint get_cluster_index(float3 world_position)
{
    float4x4 vp_matrix = mul(projection_matrix, view_matrix);
    float4 view_space_pos = clip_space_to_viewport_space(
        mul(vp_matrix, float4(world_position, 1.0f)), 
        view_dimensions
    );

    float2 tile_size = ceil(view_dimensions / light_grid_size);
    return get_light_cluster_index(view_space_pos.xyz, tile_size, light_grid_size, view_z_near, view_z_far);
}

// ================================================================================================
//  The all powerful shade function!
// ================================================================================================

float4 shade_fragment(gbuffer_fragment frag, bool is_transparent_or_ray)
{
    float3 final_color = 0.0f;
    float3 ambient_lighting = 0.0f;
    float3 direct_lighting = 0.0f;
    int max_cascade = 0;
    uint cluster_index = 0;
    uint visible_light_count = 0;

    if ((frag.flags & render_gpu_flags::unlit) == 0)
    {
        // Calculate ambient lighting from our light probes.
        ambient_lighting = calculate_ambient_lighting(frag, is_transparent_or_ray);

        // Calculate direct contribution of all lights that effect us.
        direct_lighting = float3(0.0, 0.0, 0.0);
        max_cascade = 0;

#ifndef DO_NOT_USE_LIGHT_CLUSTERS    
        // Calculate the cluser we are contained inside of.
        cluster_index = get_cluster_index(frag.world_position.xyz);
        light_cluster cluster = light_cluster_buffer.Load<light_cluster>(cluster_index * sizeof(light_cluster));
        visible_light_count = cluster.visible_light_count;

        // Go through all effecting lights.
        for (int i = 0; i < cluster.visible_light_count; i++)
        {
            uint light_index = light_cluster_visibility_buffer.Load<uint>((cluster.visible_light_offset + i) * sizeof(uint));
            light_state light = get_light_state(light_index);
            
            direct_lighting_result result = calculate_direct_lighting(frag, light, is_transparent_or_ray);
            direct_lighting += result.lighting;
            max_cascade = max(max_cascade, int(result.shadow_attenuation_and_cascade_index.g));
        }
#else
    for (int i = 0; i < light_count; i++)
    {
        light_state light = get_light_state(i);

        float distance = length(light.position - frag.world_position.xyz);
        if (distance < light.range && distance < light.importance_distance)
        {
            visible_light_count++;

            direct_lighting_result result = calculate_direct_lighting(frag, light, is_transparent_or_ray);
            direct_lighting += result.lighting;
            max_cascade = max(max_cascade, int(result.shadow_attenuation_and_cascade_index.g));
        }
    }

#endif

        // Mix final color.
        final_color = ambient_lighting + direct_lighting;
    }
    else
    {
        final_color = frag.albedo;
    }

#ifndef DISABLE_VISUALIZATION_MODES
    // If we are in a visualization mode, tint colors as appropriate.
    switch (visualization_mode)
    {
        default:
        case visualization_mode_t::lighting:
        case visualization_mode_t::normal:
        case visualization_mode_t::light_probes:    
        {
            // Use standard fragment color.
            break;
        }
        case visualization_mode_t::albedo:
        {
            final_color = float4(frag.albedo, 1.0f);
            break;
        }
        case visualization_mode_t::metallic:
        {
            final_color = float4(frag.metallic, 0.0f, 0.0f, 1.0f);
            break;
        }
        case visualization_mode_t::roughness:
        {
            final_color = float4(frag.roughness, 0.0f, 0.0f, 1.0f);
            break;
        }
        case visualization_mode_t::world_normal:
        {
            final_color = float4(pack_normal(frag.world_normal), 1.0f);
            break;
        }
        case visualization_mode_t::world_position:
        {
            final_color = float4(frag.world_position, 1.0f);
            break;
        }
        case visualization_mode_t::shadow_cascades:
        {
            final_color = (final_color + 0.02) * debug_colors[max_cascade % 8];
            break;
        }
        case visualization_mode_t::light_clusters:
        {
            final_color = (final_color + 0.02) * debug_colors[cluster_index % 7];
            break;
        }
        case visualization_mode_t::light_heatmap:  
        {
            final_color = (final_color * 0.2) + lerp(
                float3(0.0f, 1.0f, 0.0f),
                float3(1.0f, 0.0f, 0.0f),
                saturate(visible_light_count / float(MAX_LIGHTS_PER_CLUSTER))
            ) * 0.8;
            break;
        }
        case visualization_mode_t::light_probe_contribution:             
        {
            final_color = sample_light_probe_grids(light_probe_grid_count, light_probe_grid_buffer, frag.world_position, frag.world_normal);
            break;
        }
        case visualization_mode_t::indirect_specular:
        {
            final_color = ambient_lighting;
            break;
        }
        case visualization_mode_t::indirect_diffuse:    
        {
            final_color = ambient_lighting;
            break;
        }
        case visualization_mode_t::ao:    
        {
            final_color = ambient_lighting;
            break;
        }
        case visualization_mode_t::direct_light:
        {
            final_color = direct_lighting;
            break;
        }
    }
#endif

    return float4(final_color, 1.0f);
}

#endif // _LIGHTING_SHADING_HLSL_
