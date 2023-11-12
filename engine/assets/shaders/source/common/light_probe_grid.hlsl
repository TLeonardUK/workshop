// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _LIGHT_PROBE_GRID_HLSL_
#define _LIGHT_PROBE_GRID_HLSL_

#include "data:shaders/source/common/sh.hlsl"
#include "data:shaders/source/common/octahedral.hlsl"

// Determines what the current classification state of a probe is.
enum class probe_classification
{
    inactive,
    in_geometry,
    active
};

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

// Gets the index of a probe at the given coordinate in a grid of the given size.
int get_probe_index(int3 probe_coord, int3 grid_size)
{
    return ((grid_size.x * grid_size.y) * probe_coord.z) +
            (grid_size.x * probe_coord.y) +
            probe_coord.x;
}

// Turns a probe index into a grid of the given size in a 3 dimensional coordinate
int3 get_probe_grid_coord(int probe_index, int3 grid_size)
{
    int3 ret = int3(0, 0, 0);

    ret.z = probe_index / (grid_size.x * grid_size.y);
    probe_index -= ret.z * (grid_size.x * grid_size.y);

    ret.y = probe_index / (grid_size.x);
    probe_index -= ret.y * (grid_size.x);

    ret.x = probe_index;

    return ret;
}

// Gets the coordinate in the atlas, in probe space not in pixel space.
int2 get_probe_atlas_coords(int probe_index, int probes_per_row, int map_size)
{
    return int2(probe_index % probes_per_row, probe_index / probes_per_row);
}

// Gets the UV's in for a probes data in the occlusion/irradiance atlases.
float2 get_probe_uv(int3 probe_coord, int3 grid_size, float2 octant_coordinates, int map_size, int texture_size, int probes_per_row, light_probe_grid_state grid_state)
{
    int probe_index = get_probe_index(probe_coord, grid_size);

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

// Gets the persistent probe state, which
// contains information like relocation / classification data.
light_probe_state get_light_probe_state(int probe_state_buffer_index, int probe_index)
{
    light_probe_state probe_data = table_rw_byte_buffers[NonUniformResourceIndex(probe_state_buffer_index)].Load<light_probe_state>(probe_index * sizeof(light_probe_state));
    return probe_data;
}
void set_light_probe_state(int probe_state_buffer_index, int probe_index, light_probe_state state)
{
    table_rw_byte_buffers[NonUniformResourceIndex(probe_state_buffer_index)].Store(probe_index * sizeof(light_probe_state), state);
}

// Gets a bias from the surface of a shaded fragment for sampling
// a light probe field. Helps handle numeric instability.
float3 get_light_probe_grid_surface_bias(float3 surface_normal, float3 camera_direction, float normal_bias, float view_bias)
{
    return (surface_normal * normal_bias) + (-camera_direction * view_bias);
}

// Get probe world space position.
float3 get_light_probe_world_position(light_probe_grid_state grid_state, int probe_index)
{
    light_probe_state probe_state = get_light_probe_state(grid_state.probe_state_buffer_index, probe_index);

    float3 half_bounds = grid_state.bounds * 0.5f;

    int3 grid_coord = get_probe_grid_coord(probe_index, grid_state.size);

    float3 probe_local_position = (grid_coord * grid_state.density);
    float3 probe_world_position = mul(grid_state.grid_to_world_matrix, float4(probe_local_position - half_bounds, 1.0f)).xyz + probe_state.position_offset.xyz;

    return probe_world_position;
}


// Samples a given world position from the first light probe grid thats
// found to contain the given position.
//
// Bunch of this is based on this DDGI presentation / RTXGI implementation:
// https://www.gdcvault.com/play/1026182/
float3 sample_light_probe_grids(int grid_count, ByteAddressBuffer grid_buffer, float3 world_position, float3 world_normal, float3 view_direction)
{
    // Find the grid that contains this point. We always take the first one
    // we don't attempt to blend between grids.
    for (uint i = 0; i < grid_count; i++)
    {
        // Load information on the grid and how its constructed.
        instance_offset_info info = grid_buffer.Load<instance_offset_info>(i * sizeof(instance_offset_info));
        light_probe_grid_state grid_state = table_byte_buffers[NonUniformResourceIndex(info.data_buffer_index)].Load<light_probe_grid_state>(info.data_buffer_offset);

        // Determine the surface biased world position we should actually attempt to sample from.
        float3 half_bounds = grid_state.bounds * 0.5f;
        float3 surface_bias = get_light_probe_grid_surface_bias(world_normal, view_direction, grid_state.normal_bias, grid_state.view_bias);
        float3 biased_world_position = world_position + surface_bias;

        // Translate world position to grid space which makes various lookups simpler.
        float3 local_position = mul(grid_state.world_to_grid_matrix, float4(world_position, 1.0f)).xyz;
        float3 local_normal = normalize(mul(grid_state.world_to_grid_matrix, float4(world_normal, 0.0f))).xyz;

        // Check the local position is contained within the grid bounds.
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

            // Calculate the coordinates of all adjacent probes within the grid.
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

            // Sample all adjacent probes and accumulate their irradiance contribution.
            float4 irradiance = float4(0.0f, 0.0f, 0.0f, 0.0f);
            for (int i = 0; i < 8; i++)
            {
                int3 grid_coord = sample_locations[i];
                int3 probe_offset = grid_coord - min_coords;

                // Load the persistent state of each probe, we need it to get information such as
                // its world position offset.
                int probe_index = get_probe_index(grid_coord, grid_state.size);
                light_probe_state probe_state = get_light_probe_state(grid_state.probe_state_buffer_index, probe_index);

                // Skip any inactive probes.
                if (probe_state.classification != (int)probe_classification::active)
                {
                    continue;
                }

                // Calculate the world position of this probe.
                float3 probe_local_position = (grid_coord * grid_state.density);
                float3 probe_world_position = mul(grid_state.grid_to_world_matrix, float4(probe_local_position - half_bounds, 1.0f)).xyz + probe_state.position_offset.xyz;

                // General vectors used in calculations.
                float3 unbiased_probe_to_frag = normalize(probe_world_position - world_position);    
                float3 probe_to_frag = normalize(probe_world_position - biased_world_position);    
                float  probe_to_frag_distance = length(probe_world_position - biased_world_position);    
                float weight = 1.0f;

                // Smooth backface
                float wrap_shading = (dot(probe_to_frag, world_normal) + 1) * 0.5f;
                weight += (wrap_shading * wrap_shading) * 0.2f;

                // Calculate trilinear weight from adjacency.
                float3 trilinear_weight3 = lerp(float3(1.0f, 1.0f, 1.0f) - delta, delta, float3(probe_offset.x, probe_offset.y, probe_offset.z));
                float trilinear_weight = (trilinear_weight3.x * trilinear_weight3.y * trilinear_weight3.z);

                // Sample occlusion distance.
                float2 occlusion_octant_coords = get_octahedral_coordinates(-probe_to_frag);
                float2 occlusion_probe_texture_uv = get_probe_uv(grid_coord, grid_state.size, occlusion_octant_coords, grid_state.occlusion_map_size, grid_state.occlusion_texture_size, grid_state.occlusion_probes_per_row, grid_state);

                float2 filtered_distance = 2.f * occlusion_texture_map.SampleLevel(map_sampler, occlusion_probe_texture_uv, 0).rg;
                float distance_variance = abs((filtered_distance.x * filtered_distance.x) - filtered_distance.y);

                // Chebyshev occlusion test.
                float chebyshev_weight = 1.0f;
                if (probe_to_frag_distance > filtered_distance.x)
                {
                    float v = probe_to_frag_distance - filtered_distance.x;
                    chebyshev_weight = distance_variance / (distance_variance + (v * v));
                    chebyshev_weight = max((chebyshev_weight * chebyshev_weight * chebyshev_weight), 0.0f);
                }            
                weight *= max(0.05f, chebyshev_weight);

                // Avoid zero weights.
                weight = max(0.000001f, weight);

                // Crush small amounts of lights
                const float crush_threshold = 0.2f;
                if (weight < crush_threshold)
                {
                    weight *= (weight * weight) * (1.0f / (crush_threshold * crush_threshold));
                }

                // Apply trilinear weighting.
                weight *= trilinear_weight;

                // Sample irradiance.
                float2 irradiance_octant_coords = get_octahedral_coordinates(world_normal);
                float2 irradiance_probe_texture_uv = get_probe_uv(grid_coord, grid_state.size, irradiance_octant_coords, grid_state.irradiance_map_size, grid_state.irradiance_texture_size, grid_state.irradiance_probes_per_row, grid_state);
                float3 irradiance_sample = irradiance_texture_map.SampleLevel(map_sampler, irradiance_probe_texture_uv, 0).rgb;

                // Accumulate result.
                irradiance += float4(irradiance_sample, 1.0f) * weight;
                //irradiance += float4(filtered_distance.x / 150.0f, 0.0f, 0.0f, 1.0f);
                //irradiance += float4(weight, weight, weight, 1.0f) * weight;
            }

            // No probe data, return black.
            if (abs(irradiance.a) < 0.0001f)
            {
                return float3(0.0f, 0.0f, 0.0f);
            }

            // Normalize weights.
            irradiance.rgb = irradiance.rgb / irradiance.a;
            
            return irradiance.rgb;
        }    
    }

    return float3(0.0f, 0.0f, 0.0f);
}

#endif // _LIGHT_PROBE_GRID_HLSL_
