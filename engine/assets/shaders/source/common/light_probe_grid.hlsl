// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _LIGHT_PROBE_GRID_HLSL_
#define _LIGHT_PROBE_GRID_HLSL_

#include "data:shaders/source/common/sh.hlsl"
#include "data:shaders/source/common/octahedral.hlsl"

// Goes through a buffer full of light probe grid states and selects the one appropriate for the given world position.
// Returns -1 if world position is not inside any grid.
int find_intersecting_probe_grid(int grid_count, ByteAddressBuffer grid_buffer, float3 world_position)
{
    for (uint i = 0; i < grid_count; i++)
    {
        instance_offset_info info = grid_buffer.Load<instance_offset_info>(i * sizeof(instance_offset_info));
        light_probe_grid_state grid_state = table_byte_buffers[NonUniformResourceIndex(info.data_buffer_index)].Load<light_probe_grid_state>(info.data_buffer_offset);

        float3 half_bounds = grid_state.bounds * 0.5f;

        float3 local_position = mul(grid_state.world_to_grid_matrix, float4(world_position, 1.0f)).xyz;
        if (local_position.x >= -half_bounds.x && local_position.x < half_bounds.x &&
            local_position.y >= -half_bounds.y && local_position.y < half_bounds.y &&
            local_position.z >= -half_bounds.z && local_position.z < half_bounds.z)
        {
            return i;
        }
    }

    return -1;
}

int2 get_probe_atlas_coords(int probe_index, int probes_per_row, int map_size)
{
    return int2(probe_index % probes_per_row, probe_index / probes_per_row);
}

float2 get_probe_uv(int3 probe_coord, int3 grid_size, float2 octant_coordinates, int map_size, int texture_size, int probes_per_row, light_probe_grid_state grid_state)
{
    int probe_index = 
        ((grid_size.x * grid_size.y) * probe_coord.z) +
        (grid_size.x * probe_coord.y) +
        probe_coord.x;

    // Get the probe's texel coordinates, assuming one texel per probe
    uint2 coords = get_probe_atlas_coords(probe_index, probes_per_row, map_size);

    // Add the border texels to get the total texels per probe
    float num_probe_texels = (map_size + 2.0f);

    // Move to the center of the probe and move to the octant texel before normalizing
    float2 uv = float2(coords.x * num_probe_texels, coords.y * num_probe_texels) + (num_probe_texels * 0.5f);
    uv += octant_coordinates.xy * ((float)map_size * 0.50f);
    uv /= float2(texture_size, texture_size);

    return uv;
}

// Based on DDGI presentation:
// https://www.gdcvault.com/play/1026182/

float3 sample_light_probe_grids(int grid_count, ByteAddressBuffer grid_buffer, float3 world_position, float3 world_normal)
{
    for (uint i = 0; i < grid_count; i++)
    {
        instance_offset_info info = grid_buffer.Load<instance_offset_info>(i * sizeof(instance_offset_info));
        light_probe_grid_state grid_state = table_byte_buffers[NonUniformResourceIndex(info.data_buffer_index)].Load<light_probe_grid_state>(info.data_buffer_offset);

        float3 half_bounds = grid_state.bounds * 0.5f;

        float3 local_position = mul(grid_state.world_to_grid_matrix, float4(world_position, 1.0f)).xyz;
        float3 local_normal = normalize(mul(grid_state.world_to_grid_matrix, float4(world_normal, 0.0f))).xyz;

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
            //float3 delta = saturate((local_position - (min_coords * grid_state.density)) / grid_state.density);
            float3 delta = local_position / grid_state.density - min_coords;

            // Sample the 8 probes surrounding the position.
            Texture2D irradiance_texture_map = table_texture_2d[NonUniformResourceIndex(grid_state.irradiance_texture_index)];
            Texture2D occlusion_texture_map = table_texture_2d[NonUniformResourceIndex(grid_state.occlusion_texture_index)];
            sampler map_sampler = table_samplers[NonUniformResourceIndex(grid_state.map_sampler_index)];

            int3 sample_locations[] = {
                int3(min_coords.x, min_coords.y, min_coords.z),
                int3(max_coords.x, min_coords.y, min_coords.z),
                int3(min_coords.x, max_coords.y, min_coords.z),
                int3(max_coords.x, max_coords.y, min_coords.z),

                int3(min_coords.x, min_coords.y, max_coords.z),
                int3(max_coords.x, min_coords.y, max_coords.z),
                int3(min_coords.x, max_coords.y, max_coords.z),
                int3(max_coords.x, max_coords.y, max_coords.z),
            };

            float4 irradiance = float4(0.0f, 0.0f, 0.0f, 0.0f);
            for (int i = 0; i < 8; i++)
            {
                int3 grid_coord = sample_locations[i];
                int3 probe_offset = grid_coord - min_coords;

                float3 local_probe_position = (grid_coord * grid_state.density);
                float3 local_probe_to_frag = normalize(local_probe_position - local_position);                

                // Smooth backface
                /*
                float d = dot(local_probe_to_frag, local_normal);
                float weight = 1.0f;
                if (d <= 0.0f)
                {
                    continue;
                }
                if (d <= 0.2f)
                {
                    weight = inverse_lerp(0.0f, 0.2f, d);
                }*/
                float weight = 1.0f;

                // Adjacency
                float3 trilinear_weight3 = lerp(float3(1.0f, 1.0f, 1.0f) - delta, delta, float3(probe_offset.x, probe_offset.y, probe_offset.z));
                weight *= (trilinear_weight3.x * trilinear_weight3.y * trilinear_weight3.z);

                // Visibility (Chebyshev)
                // TODO
            
                if (weight > 0.0f)
                {
                    // Sample irradiance map.
                    float2 octant_coords = get_octahedral_coordinates(world_normal);
                    float2 probe_texture_uv = get_probe_uv(grid_coord, grid_state.size, octant_coords, grid_state.irradiance_map_size, grid_state.irradiance_texture_size, grid_state.irradiance_probes_per_row, grid_state);
                    irradiance += float4(irradiance_texture_map.SampleLevel(map_sampler, probe_texture_uv, 0).rgb, 1.0f) * weight;
                }
            }

            if (abs(irradiance.a) < 0.0001f)
            {
                return float3(0.0f, 0.0f, 0.0f);
            }
            
            return irradiance.rgb / irradiance.a;
        }    
    }

    return float3(0.0f, 0.0f, 0.0f);
}

#endif // _LIGHT_PROBE_GRID_HLSL_
