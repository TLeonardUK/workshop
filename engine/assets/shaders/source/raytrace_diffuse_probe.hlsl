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

[shader("raygeneration")]
void ray_generation()
{
    uint launch_index = DispatchRaysIndex().x;

    // Generate a puedo-random population of ray normals
    float3 ray_normal = spherical_fibonacci(launch_index, probe_ray_count);

    // Trace scene and get result.
    raytrace_scene_result result = raytrace_scene(
        scene_tlas,
        probe_origin, 
        ray_normal,
        probe_far_z
    );

    // Store result for later combining.
    raytrace_diffuse_probe_scrach_data scratch;
    scratch.color = result.color;
    scratch.normal = ray_normal;
    scratch.distance = result.depth;
    scratch.valid = result.hit;
    scratch_buffer.Store<raytrace_diffuse_probe_scrach_data>(launch_index * sizeof(raytrace_diffuse_probe_scrach_data), scratch);
} 

//#define USE_FAKE_RAYS 1

float4 calculate_irradiance_for_texel(int2 texel_location)
{
    float2 probe_octant_uv = get_normalized_octahedral_coordinates(texel_location, irradiance_map_size);
    float3 probe_ray_direction = get_octahedral_direction(probe_octant_uv);

    float4 result = float4(0.0f, 0.0f, 0.0f, 0.0f);

#ifdef USE_FAKE_RAYS
    int ray_count = 2;
    raytrace_diffuse_probe_scrach_data fake_rays[2];
    fake_rays[0].color = float3(1.0f, 0.0f, 0.0f);
    fake_rays[0].normal = float3(1.0f, 0.0f, 0.0f);
    fake_rays[0].distance = 0.0f;
    fake_rays[0].valid = true;
    
    fake_rays[1].color = float3(0.0f, 1.0f, 0.0f);
    fake_rays[1].normal = float3(-1.0f, 0.0f, 0.0f);
    fake_rays[1].distance = 0.0f;
    fake_rays[1].valid = true;
    
    /*fake_rays[2].color = float3(1.0f, 0.0f, 1.0f);
    fake_rays[2].normal = float3(1.0f, 0.0f, 0.0f);
    fake_rays[2].distance = 0.0f;
    fake_rays[2].valid = true;
    
    fake_rays[3].color = float3(1.0f, 1.0f, 0.0f);
    fake_rays[3].normal = float3(-1.0f, 0.0f, 0.0f);
    fake_rays[3].distance = 0.0f;
    fake_rays[3].valid = true;*/
#else
    int ray_count = probe_ray_count;
#endif

    for (int i = 0; i < ray_count; i++)
    {
#if USE_FAKE_RAYS
        raytrace_diffuse_probe_scrach_data ray_result = fake_rays[i];
#else
        raytrace_diffuse_probe_scrach_data ray_result = scratch_buffer.Load<raytrace_diffuse_probe_scrach_data>(i * sizeof(raytrace_diffuse_probe_scrach_data));
#endif
        if (ray_result.valid)
        {
            float weight = max(0.0f, dot(probe_ray_direction, ray_result.normal));            
            result += float4(ray_result.color * weight, weight);
        }
    }

    float epsilon = float(ray_count);
    epsilon *= 1e-9f;

    // Normalize the blended value. Read the RTXGI documentation to get an idea of how this works.
    //result.rgb *= 1.f / (2.f * max(result.a, epsilon));
    result.rgb *= 1.f / max(result.a, epsilon);
    result.a = 1.f;

    return result;
}

float4 calculate_occlusion_for_texel(int2 texel_location)
{
    float2 probe_octant_uv = get_normalized_octahedral_coordinates(texel_location, occlusion_map_size);
    float3 probe_ray_direction = get_octahedral_direction(probe_octant_uv);

    float4 result = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float max_distance = probe_spacing * 1.5f;

    for (int i = 0; i < probe_ray_count; i++)
    {
        raytrace_diffuse_probe_scrach_data ray_result = scratch_buffer.Load<raytrace_diffuse_probe_scrach_data>(i * sizeof(raytrace_diffuse_probe_scrach_data));
        if (ray_result.valid)
        {
            // Determine contribute of ray to output texel.
            float weight = max(0.0f, dot(probe_ray_direction, ray_result.normal));            

            // Apply sharpness to weight.
            weight = pow(weight, probe_distance_exponent);

            // Retrieve and accumulate the distance.
            float distance = min(abs(ray_result.distance), max_distance);
            result += float4(distance * weight, (distance * distance) * weight, 0.0f, weight);
        }
    }

    float epsilon = float(probe_ray_count);
    epsilon *= 1e-9f;

    // Normalize the blended value. Read the RTXGI documentation to get an idea of how this works.
    //result.rgb *= 1.f / (2.f * max(result.a, epsilon));
    result.rgb *= 1.f / max(result.a, epsilon);
    result.a = 1.f;

    return result;
}

void update_border_texels(RWTexture2D<float4> texture, int2 atlas_coord, int2 probe_location, int map_size)
{    
    int num_texels = map_size + 2;

    for (int y = 0; y < num_texels; y++)
    {
        for (int x = 0; x < num_texels; x++)
        {
            bool is_border = (x == 0 || y == 0 || x == num_texels - 1 || y == num_texels - 1);

            if (is_border)
            {
                bool is_corner = (x == 0 || x == num_texels - 1) && (y == 0 || y == num_texels - 1);
                bool is_row = (x > 0 && x < num_texels - 1);

                int2 copy_coord = int2(atlas_coord.x * num_texels, atlas_coord.y * num_texels);
                if (is_corner)
                {
                    copy_coord.x += (x > 0 ? 1 : map_size);
                    copy_coord.y += (y > 0 ? 1 : map_size);
                }
                else if (is_row)
                {
                    copy_coord.x += (num_texels - 1) - x;
                    copy_coord.y += y + ((y > 0) ? -1 : 1);
                }
                else
                {
                    copy_coord.x += x + ((x > 0) ? -1 : 1);
                    copy_coord.y += (num_texels - 1) - y;
                }

                texture[probe_location + int2(x, y)] = texture[copy_coord];
            }
        }
    }
}

[numthreads(1, 1, 1)]
void combine_output_cshader()
{
    // TODO: Add a thread for each output texel.

    // Store irradiance texture.
    int map_side_padding = 1;
    int map_padding = map_side_padding * 2;
    int full_irradiance_map_size = irradiance_map_size + map_padding;

    int2 irradiance_atlas_coord = int2(probe_index % irradiance_per_row, probe_index / irradiance_per_row);
    int2 irradiance_grid_location = int2(probe_index % irradiance_per_row, probe_index / irradiance_per_row) * int2(full_irradiance_map_size, full_irradiance_map_size);
    for (int y = 0; y < irradiance_map_size; y++)
    {
        for (int x = 0; x < irradiance_map_size; x++)
        {
            int2 offset = int2(x + map_side_padding, y + map_side_padding);
            irradiance_texture[irradiance_grid_location + offset] = calculate_irradiance_for_texel(int2(x, y));
        }
    }

    // Update irradiance border texels.
    update_border_texels(irradiance_texture, irradiance_atlas_coord, irradiance_grid_location, irradiance_map_size);

    // Store occlusion texture.
    int full_occlusion_map_size = occlusion_map_size + map_padding;
    int2 occlusion_atlas_coord =  int2(probe_index % occlusion_per_row, probe_index / occlusion_per_row);
    int2 occlusion_grid_location = int2(probe_index % occlusion_per_row, probe_index / occlusion_per_row) * int2(full_occlusion_map_size, full_occlusion_map_size);
    for (int y = 0; y < occlusion_map_size; y++)
    {
        for (int x = 0; x < occlusion_map_size; x++)
        {
            int2 offset = int2(x + map_side_padding, y + map_side_padding);
            occlusion_texture[occlusion_grid_location + offset] = calculate_occlusion_for_texel(int2(x, y));
        }
    }
    
    // Update occlusion border texels.
    update_border_texels(occlusion_texture, occlusion_atlas_coord, occlusion_grid_location, occlusion_map_size);
}