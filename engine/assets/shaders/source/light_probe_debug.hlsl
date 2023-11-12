// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/common/vertex.hlsl"
#include "data:shaders/source/common/sh.hlsl"
#include "data:shaders/source/common/light_probe_grid.hlsl"

struct light_probe_pinput
{
    float4 position : SV_POSITION;
    float4 world_normal : NORMAL1;

    int3 probe_grid_coord : TEXCOORD2;
    int2 grid_state_location : TEXCOORD3;
    probe_classification classification : TEXCOORD4;
};

struct light_probe_poutput
{
    float4 color : SV_Target0;
};

light_probe_pinput vshader(vertex_input input)
{
    vertex v = load_vertex(input.vertex_id);
    light_probe_instance_info vi = load_light_probe_instance_info(input.instance_id);

    float4x4 mvp_matrix = mul(projection_matrix, mul(view_matrix, vi.model_matrix));

    // Offset vertex by world position offset
    light_probe_grid_state grid_state = table_byte_buffers[NonUniformResourceIndex(vi.grid_state_table_index)].Load<light_probe_grid_state>(vi.grid_state_table_offset);
    int probe_index = get_probe_index(vi.grid_coord, grid_state.size);
    light_probe_state probe_state = get_light_probe_state(grid_state.probe_state_buffer_index, probe_index);

    light_probe_pinput result;
    result.position = mul(mvp_matrix, float4(v.position + (probe_state.position_offset / vi.scale), 1.0f));
    result.world_normal = mul(vi.model_matrix, float4(v.normal, 0.0f));
    result.probe_grid_coord = vi.grid_coord;
    result.grid_state_location = uint2(vi.grid_state_table_index, vi.grid_state_table_offset);
    result.classification = (probe_classification)probe_state.classification;

    return result;
}

light_probe_poutput pshader(light_probe_pinput input)
{
    light_probe_poutput output;

    if (input.classification == probe_classification::inactive)
    {
        discard;
    }

    float3 sample_normal = normalize(input.world_normal).xyz;

    light_probe_grid_state grid_state = table_byte_buffers[NonUniformResourceIndex(input.grid_state_location[0])].Load<light_probe_grid_state>(input.grid_state_location[1]);

    Texture2D irradiance_texture_map = table_texture_2d[NonUniformResourceIndex(grid_state.irradiance_texture_index)];
    Texture2D occlusion_texture_map = table_texture_2d[NonUniformResourceIndex(grid_state.occlusion_texture_index)];
    sampler map_sampler = table_samplers[NonUniformResourceIndex(grid_state.map_sampler_index)];

    if (input.classification == probe_classification::active)
    {
        float2 octant_coords = get_octahedral_coordinates(sample_normal);
#if 1        
        float2 probe_texture_uv = get_probe_uv(input.probe_grid_coord, grid_state.size, octant_coords, grid_state.irradiance_map_size, grid_state.irradiance_texture_size, grid_state.irradiance_probes_per_row, grid_state);
        float3 diffuse = irradiance_texture_map.SampleLevel(map_sampler, probe_texture_uv, 0).rgb;
#else
        float2 probe_texture_uv = get_probe_uv(input.probe_grid_coord, grid_state.size, octant_coords, grid_state.occlusion_map_size, grid_state.occlusion_texture_size, grid_state.occlusion_probes_per_row, grid_state);
        float3 diffuse = occlusion_texture_map.SampleLevel(map_sampler, probe_texture_uv, 0).rgb;
        diffuse.rgb = diffuse.r / 150.0f;
#endif

        output.color = float4(diffuse, 1.0f);
    }        
    else if (input.classification == probe_classification::in_geometry)
    {
        output.color = float4(0.0f, 1.0f, 0.0f, 1.0f);
    }
    else 
    {
        output.color = float4(1.0f, 0.0f, 0.0f, 1.0f);
    }

    return output;
}