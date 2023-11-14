// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================

#include "data:shaders/source/common/vertex.hlsl"
#include "data:shaders/source/common/math.hlsl"
#include "data:shaders/source/common/octahedral.hlsl"
#include "data:shaders/source/common/sh.hlsl"
#include "data:shaders/source/common/lighting.hlsl"
#include "data:shaders/source/common/raytracing.hlsl"
#include "data:shaders/source/common/raytracing_primitive_ray.hlsl"
#include "data:shaders/source/common/raytracing_occlusion_ray.hlsl"
#include "data:shaders/source/common/raytracing_scene.hlsl"

float4 calculate_irradiance_for_texel(int2 texel_location, uint probe_offset, ddgi_probe_data probe_data)
{
    float2 probe_octant_uv = get_normalized_octahedral_coordinates(texel_location, probe_data.irradiance_map_size);
    float3 probe_ray_direction = get_octahedral_direction(probe_octant_uv);

    float4 result = float4(0.0f, 0.0f, 0.0f, 0.0f);
    int ray_count = probe_ray_count;

    const float probe_backface_threshold = 0.1f;
    uint backface_count = 0;

    for (int i = 0; i < ray_count; i++)
    {
        int ray_index = probe_offset + i;
        ddgi_probe_scrach_data ray_result = scratch_buffer.Load<ddgi_probe_scrach_data>(ray_index * sizeof(ddgi_probe_scrach_data));

        // Don't blend backfaces into the result.
        if (ray_result.distance < 0.f)
        {
            continue;
        }

        float weight = max(0.0f, dot(probe_ray_direction, ray_result.normal));            
        result += float4(ray_result.color * weight, weight);
    }

    // Fixed rays are not blended in.
    float epsilon = float(ray_count - PROBE_GRID_FIXED_RAY_COUNT);
    epsilon *= 1e-9f;

    // Normalize the blended value. Read the RTXGI documentation to get an idea of how this works.
    result.rgb *= 1.f / (2.f * max(result.a, epsilon));
    result.a = 1.f;

    // Apply tone mapping to result
    result.rgb = pow(result.rgb, (1.0f / probe_encoding_gamma));

    return result;
}

float4 calculate_occlusion_for_texel(int2 texel_location, uint ray_offset, ddgi_probe_data probe_data)
{
    float2 probe_octant_uv = get_normalized_octahedral_coordinates(texel_location, probe_data.occlusion_map_size);
    float3 probe_ray_direction = get_octahedral_direction(probe_octant_uv);

    float4 result = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float max_distance = probe_data.probe_spacing * 1.5f;

    for (int i = 0; i < probe_ray_count; i++)
    {
        int ray_index = ray_offset + i;
        ddgi_probe_scrach_data ray_result = scratch_buffer.Load<ddgi_probe_scrach_data>(ray_index * sizeof(ddgi_probe_scrach_data));

        // Determine contribute of ray to output texel.
        float weight = max(0.0f, dot(probe_ray_direction, ray_result.normal));            

        // Apply sharpness to weight.
        weight = pow(weight, probe_distance_exponent);

        // Retrieve and accumulate the distance.
        float distance = min(abs(ray_result.distance), max_distance);
        result += float4(distance * weight, (distance * distance) * weight, 0.0f, weight);
    }

    // Fixed rays are not blended in.
    float epsilon = float(probe_ray_count - PROBE_GRID_FIXED_RAY_COUNT);
    epsilon *= 1e-9f;

    // Normalize the blended value. Read the RTXGI documentation to get an idea of how this works.
    result.rgb *= 1.f / (2.f * max(result.a, epsilon));
    result.a = 1.f;

    return result;
}

void update_border_texels(RWTexture2D<float4> texture, int2 atlas_coord, int2 probe_location, int map_size, int2 texel_pos)
{    
    int num_texels = map_size + 2;

    bool is_corner = (texel_pos.x == 0 || texel_pos.x == num_texels - 1) && (texel_pos.y == 0 || texel_pos.y == num_texels - 1);
    bool is_row = (texel_pos.x > 0 && texel_pos.x < num_texels - 1);

    int2 copy_coord = probe_location;
    if (is_corner)
    {
        copy_coord.x += (texel_pos.x > 0 ? 1 : map_size);
        copy_coord.y += (texel_pos.y > 0 ? 1 : map_size);
    }
    else if (is_row)
    {
        copy_coord.x += (num_texels - 1) - texel_pos.x;
        copy_coord.y += texel_pos.y + ((texel_pos.y > 0) ? -1 : 1);
    }
    else
    {
        copy_coord.x += texel_pos.x + ((texel_pos.x > 0) ? -1 : 1);
        copy_coord.y += (num_texels - 1) - texel_pos.y;
    }

    texture[probe_location + int2(texel_pos.x, texel_pos.y)] = texture[copy_coord];
}

struct cshader_parameters
{
    uint3 group_id : SV_GroupID;
    uint3 group_thread_id : SV_GroupThreadID;
};

[numthreads(PROBE_GRID_IRRADIANCE_MAP_SIZE + 2, PROBE_GRID_IRRADIANCE_MAP_SIZE + 2, 1)]
void output_irradiance_cshader(cshader_parameters params)
{
    // Load probe data.
    instance_offset_info info = probe_data_buffer.Load<instance_offset_info>(params.group_id.x * sizeof(instance_offset_info));
    ddgi_probe_data probe_data = table_byte_buffers[NonUniformResourceIndex(info.data_buffer_index)].Load<ddgi_probe_data>(info.data_buffer_offset);
    
    RWTexture2D<float4> irradiance_texture = table_rw_texture_2d[probe_data.irradiance_texture_index];

    // Store irradiance texture.
    int full_irradiance_map_size = probe_data.irradiance_map_size + 2;

    int2 irradiance_atlas_coord = int2(probe_data.probe_index % probe_data.irradiance_per_row, probe_data.probe_index / probe_data.irradiance_per_row);
    int2 irradiance_grid_location = irradiance_atlas_coord * int2(full_irradiance_map_size, full_irradiance_map_size);
 
    // Update value of non-border pixels.
    int2 texel_coord = params.group_thread_id.xy;
    bool is_border = (texel_coord.x == 0 || texel_coord.y == 0 || texel_coord.x == full_irradiance_map_size - 1 || texel_coord.y == full_irradiance_map_size - 1);
    if (!is_border)
    {
        // Calculate new distance and store off the existing distance.
        float4 result = calculate_irradiance_for_texel(int2(texel_coord.x - 1, texel_coord.y - 1), params.group_id.x * probe_ray_count, probe_data);
        float4 existing = irradiance_texture[irradiance_grid_location + texel_coord];

#if 1
        // Blend the result with the existing value.
        float hysteresis = probe_blend_hysteresis;
        float3 delta = (result.rgb - existing.rgb);

        // If existing value is completely black, change the hysteresis to take the new
        // value immediately.        
        if (dot(existing, existing) == 0) 
        {
            hysteresis = 0.f;
        }

        // If large change is changed, reduce hysteresis to apply lighting effect quicker.
        if (maxabs3(delta) > probe_large_change_threshold)
        {
            hysteresis *= 0.75f;
        }

        // Clamp max brightness change per update.
        if (calculate_luminance(delta) > probe_brightness_threshold)
        {
            delta *= 0.25f;
        }

        // Interpolate to get the final result
        float3 lerp_delta = (1.f - hysteresis) * delta;        
        result = float4(existing.rgb + lerp_delta, 1.f);
#else
        result = float4(lerp(existing.rgb, result.rgb, 0.5f), 1.f);
#endif

        irradiance_texture[irradiance_grid_location + texel_coord] = result;
    }

    // Wait for all threads to finish before outputing borders.
    AllMemoryBarrierWithGroupSync();

    // Update border texels.
    if (is_border)
    {
        update_border_texels(irradiance_texture, irradiance_atlas_coord, irradiance_grid_location, probe_data.irradiance_map_size, texel_coord);
    }
}

[numthreads(PROBE_GRID_OCCLUSION_MAP_SIZE + 2, PROBE_GRID_OCCLUSION_MAP_SIZE + 2, 1)]
void output_occlusion_cshader(cshader_parameters params)
{
    // Load probe data.
    instance_offset_info info = probe_data_buffer.Load<instance_offset_info>(params.group_id.x * sizeof(instance_offset_info));
    ddgi_probe_data probe_data = table_byte_buffers[NonUniformResourceIndex(info.data_buffer_index)].Load<ddgi_probe_data>(info.data_buffer_offset);
    
    RWTexture2D<float4> occlusion_texture = table_rw_texture_2d[probe_data.occlusion_texture_index];

    // Store occlusion texture.
    int full_occlusion_map_size = probe_data.occlusion_map_size + 2;

    int2 occlusion_atlas_coord = int2(probe_data.probe_index % probe_data.occlusion_per_row, probe_data.probe_index / probe_data.occlusion_per_row);
    int2 occlusion_grid_location = occlusion_atlas_coord * int2(full_occlusion_map_size, full_occlusion_map_size);
    
    // Update value of non-border pixels.
    int2 texel_coord = params.group_thread_id.xy;
    bool is_border = (texel_coord.x == 0 || texel_coord.y == 0 || texel_coord.x == full_occlusion_map_size - 1 || texel_coord.y == full_occlusion_map_size - 1);
    if (!is_border)
    {
        // Calculate new distance and store off the existing distance.
        float4 result = calculate_occlusion_for_texel(int2(texel_coord.x - 1, texel_coord.y - 1), params.group_id.x * probe_ray_count, probe_data);
        float4 existing = occlusion_texture[occlusion_grid_location + texel_coord];

        // Interpolate between existing and new distance
        result = float4(lerp(result.rg, existing.rg, probe_blend_hysteresis), 0.0f, 1.0f);

        occlusion_texture[occlusion_grid_location + texel_coord] = result;
    }
    
    // Wait for all threads to finish before outputing borders.
    AllMemoryBarrierWithGroupSync();

    // Update occlusion border texels.
    if (is_border)
    {
        update_border_texels(occlusion_texture, occlusion_atlas_coord, occlusion_grid_location, probe_data.occlusion_map_size, texel_coord);
    }
}