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
void classify_cshader(cshader_parameters params)
{
    // Load probe data.
    instance_offset_info info = probe_data_buffer.Load<instance_offset_info>(params.group_id.x * sizeof(instance_offset_info));
    ddgi_probe_data probe_data = table_byte_buffers[NonUniformResourceIndex(info.data_buffer_index)].Load<ddgi_probe_data>(info.data_buffer_offset);
    
    light_probe_state probe_state = get_light_probe_state(probe_data.probe_state_buffer_index, probe_data.probe_index);
    probe_state.classification = (int)probe_classification::inactive;

    // Count the number of backface hits.
    float hit_distances[PROBE_GRID_FIXED_RAY_COUNT];
    float3 hit_directions[PROBE_GRID_FIXED_RAY_COUNT];
    int backface_count = 0;

    for (int i = 0; i < PROBE_GRID_FIXED_RAY_COUNT; i++)
    {
        int ray_index = (params.group_id.x * probe_ray_count) + i;
        ddgi_probe_scrach_data ray_result = scratch_buffer.Load<ddgi_probe_scrach_data>(ray_index * sizeof(ddgi_probe_scrach_data));

        hit_distances[i] = ray_result.distance;
        hit_directions[i] = ray_result.normal;
        backface_count += (hit_distances[i] < 0.0f);
    }

    // Check if probe is in geometry.
    if (((float)backface_count / PROBE_GRID_FIXED_RAY_COUNT) > probe_fixed_ray_backface_threshold)
    {
        probe_state.classification = (int)probe_classification::in_geometry;
    }
    else
    {
        // Get world space position of probe.
        float world_position = probe_data.probe_origin + probe_state.position_offset;

        // Determine if there is nearby geometry in the probes voxel.
        for (int ray_index = 0; ray_index < PROBE_GRID_FIXED_RAY_COUNT; ray_index++)
        {
            // Ignore backfaces
            if (hit_distances[ray_index] < 0) 
            {
                continue;
            }

            // Get the ray direction.
            float3 direction = hit_directions[ray_index];

            // Get the plane normals.
            float3 x_normal = float3(direction.x / max(abs(direction.x), 0.000001f), 0.f, 0.f);
            float3 y_normal = float3(0.f, direction.y / max(abs(direction.y), 0.000001f), 0.f);
            float3 z_normal = float3(0.f, 0.f, direction.z / max(abs(direction.z), 0.000001f));

            // Get the relevant planes to intersect.
            float3 p0x = world_position + (probe_data.probe_spacing * x_normal);
            float3 p0y = world_position + (probe_data.probe_spacing * y_normal);
            float3 p0z = world_position + (probe_data.probe_spacing * z_normal);

            // Get the ray's intersection distance with each plane
            float3 distances = float3(
                dot((p0x - world_position), x_normal) / max(dot(direction, x_normal), 0.000001f),
                dot((p0y - world_position), y_normal) / max(dot(direction, y_normal), 0.000001f),
                dot((p0z - world_position), z_normal) / max(dot(direction, z_normal), 0.000001f)
            );

            // If the ray is parallel to the plane, it will never intersect
            // Set the distance to a very large number for those planes
            if (distances.x == 0.f) 
            {
                distances.x = 1e27f;
            }
            if (distances.y == 0.f) 
            {
                distances.y = 1e27f;
            }
            if (distances.z == 0.f) 
            {
                distances.z = 1e27f;
            }

            // Get the distance to the closest plane intersection
            float max_distance = min(distances.x, min(distances.y, distances.z));

            // If the hit distance is less than the closest plane intersection, the probe should be active
            if(hit_distances[ray_index] <= max_distance)
            {
                probe_state.classification = (int)probe_classification::active;
                break;
            }
        }
    }

    // Store the new classification.
    set_light_probe_state(probe_data.probe_state_buffer_index, probe_data.probe_index, probe_state);
}
