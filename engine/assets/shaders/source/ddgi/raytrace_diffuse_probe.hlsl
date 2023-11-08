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
    uint probe_index = launch_index / probe_ray_count;
    uint probe_ray_index = launch_index % probe_ray_count;

    instance_offset_info info = probe_data_buffer.Load<instance_offset_info>(probe_index * sizeof(instance_offset_info));
    raytrace_diffuse_probe_data probe_data = table_byte_buffers[NonUniformResourceIndex(info.data_buffer_index)].Load<raytrace_diffuse_probe_data>(info.data_buffer_offset);

    // Generate a puedo-random population of ray normals
    float3 ray_normal = spherical_fibonacci(probe_ray_index, probe_ray_count);

    // Trace scene and get result.
    raytrace_scene_result result = raytrace_scene(
        scene_tlas,
        probe_data.probe_origin, 
        ray_normal,
        probe_far_z
    );

    // Store result for later combining.
    raytrace_diffuse_probe_scrach_data scratch;
    scratch.color = result.color;
    scratch.normal = ray_normal;
    scratch.distance = result.depth;
    scratch.valid = result.hit;
    scratch.back_face = (result.hit_kind == HIT_KIND_TRIANGLE_BACK_FACE);

    // Shorten distance to reduce influence during irradiance sampling, and invert
    // to track backface hits.
    if (scratch.back_face)
    {
        scratch.distance = -scratch.distance * 0.2f;
    }

    scratch_buffer.Store<raytrace_diffuse_probe_scrach_data>(launch_index * sizeof(raytrace_diffuse_probe_scrach_data), scratch);
} 

float4 calculate_irradiance_for_texel(int2 texel_location, uint probe_offset, raytrace_diffuse_probe_data probe_data)
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
        raytrace_diffuse_probe_scrach_data ray_result = scratch_buffer.Load<raytrace_diffuse_probe_scrach_data>(ray_index * sizeof(raytrace_diffuse_probe_scrach_data));
        if (ray_result.valid)
        {
            // Don't blend backfaces into the result.
            if (ray_result.distance < 0.f)
            {
                continue;
            }

            float weight = max(0.0f, dot(probe_ray_direction, ray_result.normal));            
            result += float4(ray_result.color * weight, weight);
        }
    }

    float epsilon = float(ray_count);
    epsilon *= 1e-9f;

    // Normalize the blended value. Read the RTXGI documentation to get an idea of how this works.
    result.rgb *= 1.f / (2.f * max(result.a, epsilon));
    result.a = 1.f;

    return result;
}

float4 calculate_occlusion_for_texel(int2 texel_location, uint ray_offset, raytrace_diffuse_probe_data probe_data)
{
    float2 probe_octant_uv = get_normalized_octahedral_coordinates(texel_location, probe_data.occlusion_map_size);
    float3 probe_ray_direction = get_octahedral_direction(probe_octant_uv);

    float4 result = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float max_distance = probe_data.probe_spacing * 1.5f;

    for (int i = 0; i < probe_ray_count; i++)
    {
        int ray_index = ray_offset + i;
        raytrace_diffuse_probe_scrach_data ray_result = scratch_buffer.Load<raytrace_diffuse_probe_scrach_data>(ray_index * sizeof(raytrace_diffuse_probe_scrach_data));
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

struct output_parameters
{
    uint3 group_id : SV_GroupID;
    uint3 group_thread_id : SV_GroupThreadID;
};

[numthreads(PROBE_GRID_IRRADIANCE_MAP_SIZE + 2, PROBE_GRID_IRRADIANCE_MAP_SIZE + 2, 1)]
void output_irradiance_cshader(output_parameters params)
{
    // Load probe data.
    instance_offset_info info = probe_data_buffer.Load<instance_offset_info>(params.group_id.x * sizeof(instance_offset_info));
    raytrace_diffuse_probe_data probe_data = table_byte_buffers[NonUniformResourceIndex(info.data_buffer_index)].Load<raytrace_diffuse_probe_data>(info.data_buffer_offset);
    
    RWTexture2D<float4> irradiance_texture = table_rw_texture_2d[probe_data.irradiance_texture_index];

    // Store irradiance texture.
    int full_irradiance_map_size = probe_data.irradiance_map_size + 2;

    int2 irradiance_atlas_coord = int2(probe_data.probe_index % probe_data.irradiance_per_row, probe_data.probe_index / probe_data.irradiance_per_row);
    int2 irradiance_grid_location = irradiance_atlas_coord * int2(full_irradiance_map_size, full_irradiance_map_size);
 
    int2 texel_coord = params.group_thread_id.xy;
    bool is_border = (texel_coord.x == 0 || texel_coord.y == 0 || texel_coord.x == full_irradiance_map_size - 1 || texel_coord.y == full_irradiance_map_size - 1);
    if (!is_border)
    {
        irradiance_texture[irradiance_grid_location + texel_coord] = calculate_irradiance_for_texel(int2(texel_coord.x - 1, texel_coord.y - 1), params.group_id.x * probe_ray_count, probe_data);
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
void output_occlusion_cshader(output_parameters params)
{
    // Load probe data.
    instance_offset_info info = probe_data_buffer.Load<instance_offset_info>(params.group_id.x * sizeof(instance_offset_info));
    raytrace_diffuse_probe_data probe_data = table_byte_buffers[NonUniformResourceIndex(info.data_buffer_index)].Load<raytrace_diffuse_probe_data>(info.data_buffer_offset);
    
    RWTexture2D<float4> occlusion_texture = table_rw_texture_2d[probe_data.occlusion_texture_index];

    // Store occlusion texture.
    int full_occlusion_map_size = probe_data.occlusion_map_size + 2;

    int2 occlusion_atlas_coord = int2(probe_data.probe_index % probe_data.occlusion_per_row, probe_data.probe_index / probe_data.occlusion_per_row);
    int2 occlusion_grid_location = occlusion_atlas_coord * int2(full_occlusion_map_size, full_occlusion_map_size);
    
    int2 texel_coord = params.group_thread_id.xy;
    bool is_border = (texel_coord.x == 0 || texel_coord.y == 0 || texel_coord.x == full_occlusion_map_size - 1 || texel_coord.y == full_occlusion_map_size - 1);
    if (!is_border)
    {
        occlusion_texture[occlusion_grid_location + texel_coord] = calculate_occlusion_for_texel(int2(texel_coord.x - 1, texel_coord.y - 1), params.group_id.x * probe_ray_count, probe_data);
    }
    
    // Wait for all threads to finish before outputing borders.
    AllMemoryBarrierWithGroupSync();

    // Update occlusion border texels.
    if (is_border)
    {
        update_border_texels(occlusion_texture, occlusion_atlas_coord, occlusion_grid_location, probe_data.occlusion_map_size, texel_coord);
    }
}