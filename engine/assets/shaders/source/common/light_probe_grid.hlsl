// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _LIGHT_PROBE_GRID_HLSL_
#define _LIGHT_PROBE_GRID_HLSL_

#include "data:shaders/source/common/sh.hlsl"

// Goes through a buffer full of light probe grid states and selects the one appropriate for the given world position.
// Returns -1 if world position is not inside any grid.
int find_intersecting_probe_grid(int grid_count, ByteAddressBuffer grid_buffer, float3 world_position)
{
    for (uint i = 0; i < grid_count; i++)
    {
        instance_offset_info info = grid_buffer.Load<instance_offset_info>(i * sizeof(instance_offset_info));
        light_probe_grid_state grid_state = table_byte_buffers[NonUniformResourceIndex(info.data_buffer_index)].Load<light_probe_grid_state>(info.data_buffer_offset);

        float3 half_bounds = grid_state.bounds * 0.5f;

        float3 local_position = mul(grid_state.world_to_grid_matrix, float4(world_position, 1.0f));
        if (local_position.x >= -half_bounds.x && local_position.x < half_bounds.x &&
            local_position.y >= -half_bounds.y && local_position.y < half_bounds.y &&
            local_position.z >= -half_bounds.z && local_position.z < half_bounds.z)
        {
            return i;
        }
    }

    return -1;
}

float3 sample_light_probe(RWByteAddressBuffer sh_buffer, int3 grid_size, int3 probe_coordinate, float3 world_normal)
{
    int probe_index = 
        ((grid_size.x * grid_size.y) * probe_coordinate.z) +
        (grid_size.x * probe_coordinate.y) +
        probe_coordinate.x;

    // TODO: Huuum, I don't understand how this can happen, the inputs are all clamped, but it -is- happening somehow.
    if (probe_index >= grid_size.x * grid_size.y * grid_size.z)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    uint probe_coefficient_size = 9 * 3 * sizeof(float);

    uint probe_offset = probe_index * probe_coefficient_size;

    sh_color_coefficients radiance = sh_buffer.Load<sh_color_coefficients>(probe_offset);

    return calculate_sh_irradiance(world_normal, radiance);
}

float3 sample_light_probe_grids(int grid_count, ByteAddressBuffer grid_buffer, float3 world_position, float3 world_normal)
{
    for (uint i = 0; i < grid_count; i++)
    {
        instance_offset_info info = grid_buffer.Load<instance_offset_info>(i * sizeof(instance_offset_info));
        light_probe_grid_state grid_state = table_byte_buffers[NonUniformResourceIndex(info.data_buffer_index)].Load<light_probe_grid_state>(info.data_buffer_offset);

        float3 half_bounds = grid_state.bounds * 0.5f;

        float3 local_position = mul(grid_state.world_to_grid_matrix, float4(world_position, 1.0f));
        float3 local_normal = mul(grid_state.world_to_grid_matrix, float4(world_normal, 0.0f));

        if (local_position.x >= -half_bounds.x && local_position.x < half_bounds.x &&
            local_position.y >= -half_bounds.y && local_position.y < half_bounds.y &&
            local_position.z >= -half_bounds.z && local_position.z < half_bounds.z)
        {
            int3 max_extents = grid_state.size - int3(1, 1, 1);

            // Move local position into 0-bounds range to simplify calculations.
            local_position += half_bounds;

            // Calculate the coordinates of the probes we want to use.
            int3 min_coords = int3(
                floor(local_position.x / grid_state.density),
                floor(local_position.y / grid_state.density),
                floor(local_position.z / grid_state.density)
            );
            int3 max_coords = min_coords + int3(1, 1, 1);

            // Clamp to the extends of the probe grid.
            min_coords = clamp(min_coords, int3(0, 0, 0), max_extents);
            max_coords = clamp(max_coords, int3(0, 0, 0), max_extents);

            // Find the delta position.
            float3 delta = saturate((local_position - (min_coords * grid_state.density)) / grid_state.density);

            // Sample the 8 probes surrounding the position.
            RWByteAddressBuffer sh_buffer = table_rw_byte_buffers[NonUniformResourceIndex(grid_state.sh_table_index)];

#if 0
            // TODO: maybe build cube of samples, go through them and if they are on the wrong side then replace them with a different sample
            //       then interpolate normally?

            float3 sample_locations[] = {
                int3(min_coords.x, min_coords.y, min_coords.z),
                int3(max_coords.x, min_coords.y, min_coords.z),
                int3(max_coords.x, min_coords.y, max_coords.z),
                int3(min_coords.x, min_coords.y, max_coords.z),
                int3(min_coords.x, max_coords.y, min_coords.z),
                int3(max_coords.x, max_coords.y, min_coords.z),
                int3(max_coords.x, max_coords.y, max_coords.z),
                int3(min_coords.x, max_coords.y, max_coords.z)
            };

            float3 accumulated_color = 0.0f;
            float accumulated_weight = 0.0f;

            for (int i = 0; i < 8; i++)
            {
                int3 grid_coord = sample_locations[i];
                float3 probe_position = (grid_coord * grid_state.density);
                float3 probe_to_frag = normalize(probe_position - local_position);                
                float distance = length(probe_position - local_position);

                // Make sure probe is on the correct side of fragment to try and reduce light leakage.
                //if (dot(probe_to_frag, local_normal) < 0.0f)
                {
                    float weight = saturate(1.0f - (distance / grid_state.density));
                    accumulated_color += sample_light_probe(sh_buffer, grid_state.size, grid_coord, world_normal) * weight * 2.0f;
                    accumulated_weight += weight;
                }
            }

            float3 result = abs(accumulated_weight) < 0.0001f ? 0.0f : (accumulated_color / accumulated_weight);
#else
            float3 min_x_min_y_min_z = sample_light_probe(sh_buffer, grid_state.size, int3(min_coords.x, min_coords.y, min_coords.z), world_normal);
            float3 max_x_min_y_min_z = sample_light_probe(sh_buffer, grid_state.size, int3(max_coords.x, min_coords.y, min_coords.z), world_normal);
            float3 max_x_min_y_max_z = sample_light_probe(sh_buffer, grid_state.size, int3(max_coords.x, min_coords.y, max_coords.z), world_normal);
            float3 min_x_min_y_max_z = sample_light_probe(sh_buffer, grid_state.size, int3(min_coords.x, min_coords.y, max_coords.z), world_normal);

            float3 min_x_max_y_min_z = sample_light_probe(sh_buffer, grid_state.size, int3(min_coords.x, max_coords.y, min_coords.z), world_normal);
            float3 max_x_max_y_min_z = sample_light_probe(sh_buffer, grid_state.size, int3(max_coords.x, max_coords.y, min_coords.z), world_normal);
            float3 max_x_max_y_max_z = sample_light_probe(sh_buffer, grid_state.size, int3(max_coords.x, max_coords.y, max_coords.z), world_normal);
            float3 min_x_max_y_max_z = sample_light_probe(sh_buffer, grid_state.size, int3(min_coords.x, max_coords.y, max_coords.z), world_normal);

            // Interpolate each axis seperately.
            float3 x_min_y_min_z = lerp(min_x_min_y_min_z, max_x_min_y_min_z, delta.x);
            float3 x_max_y_min_z = lerp(min_x_max_y_min_z, max_x_max_y_min_z, delta.x);
            float3 x_max_y_max_z = lerp(min_x_max_y_max_z, max_x_max_y_max_z, delta.x);
            float3 x_min_y_max_z = lerp(min_x_min_y_max_z, max_x_min_y_max_z, delta.x);

            float3 x_y_min_z = lerp(x_min_y_min_z, x_max_y_min_z, delta.y);
            float3 x_y_max_z = lerp(x_min_y_max_z, x_max_y_max_z, delta.y);
            
            float3 result = lerp(x_y_min_z, x_y_max_z, delta.z);
#endif

            return result;
        }
    }

    return float3(0.0f, 0.0f, 0.0f);
}

#endif // _LIGHT_PROBE_GRID_HLSL_
