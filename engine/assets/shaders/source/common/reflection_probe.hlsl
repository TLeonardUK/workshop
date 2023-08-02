// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _REFLECTION_PROBE_HLSL_
#define _REFLECTION_PROBE_HLSL_

float3 sample_reflection_probes(float3 world_position, float3 direction, uint probe_count, ByteAddressBuffer probe_data, float roughness)
{
    float3 result = float3(0.0f, 0.0f, 0.0f);
    float samples = 0.0f;

    for (uint i = 0; i < probe_count; i++)
    {
        instance_offset_info info = probe_data.Load<instance_offset_info>(i * sizeof(instance_offset_info));
        reflection_probe_state probe_state = table_byte_buffers[NonUniformResourceIndex(info.data_buffer_index)].Load<reflection_probe_state>(info.data_buffer_offset);

        float distance = length(probe_state.world_position - world_position);
        if (distance < probe_state.radius)
        {
            // TODO: Pre-integrate all mips off the capture.
            // TODO: Fix brdf texture black strip at top
            // TODO: Add fade-off
            // TODO: Based on roughness.
            uint mip_index = 0;

            TextureCube cube = table_texture_cube[probe_state.probe_texture_index];
            result += cube.SampleLevel(table_samplers[probe_state.probe_texture_sampler_index], direction, mip_index).rgb;
            result = float3(1.0f, 1.0f, 1.0f);
            samples += 1.0f;
        }
    }

    if (samples == 0.0f)
    {
        return result;
    }
    else
    {
        return result / samples;
    }
}

#endif // _REFLECTION_PROBE_HLSL_
