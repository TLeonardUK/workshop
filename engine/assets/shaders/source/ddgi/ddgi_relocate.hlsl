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

struct cshader_parameters
{
    uint3 group_id : SV_GroupID;
    uint3 group_thread_id : SV_GroupThreadID;
};

[numthreads(1, 1, 1)]
void relocate_cshader(cshader_parameters params)
{
    // Load probe data.
    instance_offset_info info = probe_data_buffer.Load<instance_offset_info>(params.group_id.x * sizeof(instance_offset_info));
    ddgi_probe_data probe_data = table_byte_buffers[NonUniformResourceIndex(info.data_buffer_index)].Load<ddgi_probe_data>(info.data_buffer_offset);
    
    light_probe_state probe_state = get_light_probe_state(probe_data.probe_state_buffer_index, probe_data.probe_index);

    int closest_backface_index = -1;
    int closest_frontface_index = -1;
    int farthest_frontface_index = -1;
    float closest_backface_distance = 1e27f;
    float closest_frontface_distance = 1e27f;
    float farthest_frontface_distance = 0.f;
    float backface_count = 0.f;

    // Iterate through rays and find number of back/front hits and distances between them.
    int checked_ray_count = PROBE_GRID_FIXED_RAY_COUNT;

    for (int i = 0; i < checked_ray_count; i++)
    {
        int ray_index = (params.group_id.x * probe_ray_count) + i;
        ddgi_probe_scrach_data ray_result = scratch_buffer.Load<ddgi_probe_scrach_data>(ray_index * sizeof(ddgi_probe_scrach_data));

        // Backface
        if (ray_result.distance < 0.0f)
        {
            backface_count++;

            // Scale distance back to original distance.
            ray_result.distance = ray_result.distance * -5.0f;
            if (ray_result.distance < closest_backface_distance)
            {
                closest_backface_distance = ray_result.distance;
                closest_backface_index = ray_index;
            }
        }
        // Frontface
        else
        {
            if (ray_result.distance < closest_frontface_distance)
            {
                closest_frontface_distance = ray_result.distance;
                closest_frontface_index = ray_index;
            }
            else if (ray_result.distance > farthest_frontface_distance)
            {
                farthest_frontface_distance = ray_result.distance;
                farthest_frontface_index = ray_index;
            }
        }
    }

    // Calcualte the offset.
    float3 full_offset = float3(1e27f, 1e27f, 1e27f);

    // One backface triangle is hit and backfaces hit by enough probe rays, assume inside geometry.
    if (closest_backface_index != 1 && ((float)backface_count / checked_ray_count) > probe_fixed_ray_backface_threshold)
    {
        ddgi_probe_scrach_data backface_ray = scratch_buffer.Load<ddgi_probe_scrach_data>(closest_backface_index * sizeof(ddgi_probe_scrach_data));

        float3 closest_backface_direction = backface_ray.normal;
        full_offset = probe_state.position_offset + (closest_backface_direction * (closest_backface_distance + probe_min_frontface_distance * 0.5f));
    }
    // Don't move the probe if moving towards the farthest frontface will bring it closet to the nearest.
    else if (closest_frontface_distance < probe_min_frontface_distance)
    {
        ddgi_probe_scrach_data closest_frontface_ray = scratch_buffer.Load<ddgi_probe_scrach_data>(closest_frontface_index * sizeof(ddgi_probe_scrach_data));
        ddgi_probe_scrach_data farthest_frontface_ray = scratch_buffer.Load<ddgi_probe_scrach_data>(farthest_frontface_index * sizeof(ddgi_probe_scrach_data));

        // Ensure probe never moves through the farthest frontface.
        if (dot(closest_frontface_ray.normal, farthest_frontface_ray.normal) <= 0.0f)
        {

            // TODO: Modified from the rtxgi algorithm, as the original code makes no dam sense. Opened at ticket on their github.

            // Original RTXGI code
            //farthest_frontface_distance *= min(farthest_frontface_distance, 1.0f);
            //full_offset = probe_state.position_offset + farthest_frontface_ray.normal;

            farthest_frontface_distance *= min(farthest_frontface_distance, 1.0f);
            full_offset = probe_state.position_offset + (farthest_frontface_ray.normal * 0.25f);


            // New code
            //full_offset = probe_state.position_offset + (farthest_frontface_ray.normal * max(0.0f, farthest_frontface_distance - probe_min_frontface_distance * 0.5f)); 

            //float distance_to_move = (probe_min_frontface_distance - closest_frontface_distance);
            //full_offset = probe_state.position_offset + (farthest_frontface_ray.normal * distance_to_move);
        }
    }
    // Probe not near anything, move back towards a zero offset.
    else if (closest_frontface_distance > probe_min_frontface_distance)
    {
        float move_back_margin = min(closest_frontface_distance - probe_min_frontface_distance, length(probe_state.position_offset));
        float3 move_back_direction = normalize(-probe_state.position_offset);
        full_offset = probe_state.position_offset + (move_back_margin * move_back_direction);
    }

    // Clamp distance to avoid degenerate cases.
    float3 normalized_offset = full_offset / probe_data.probe_spacing;
    if (dot(normalized_offset, normalized_offset) < 0.2025f)
    {
        probe_state.position_offset = full_offset;
    }

    // Store the new offset.
    set_light_probe_state(probe_data.probe_state_buffer_index, probe_data.probe_index, probe_state);
}
