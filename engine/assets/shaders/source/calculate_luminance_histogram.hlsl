// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/common/math.hlsl"

struct cshader_input
{
	uint3 dispatch_thread_id : SV_DispatchThreadID;
    uint group_index : SV_GroupIndex;
};

groupshared uint g_histogram_shared[HISTOGRAM_SIZE];
                        
uint calculate_bin_index(float3 color)
{
    float luminance = calculate_luminance(color);
    if (luminance < 0.001f)
    {
        return 0;
    }

    float log_luminance = saturate((log2(luminance) - min_log2_luminance) * inverse_log2_luminance_range);
    return (uint)(log_luminance * 254.0 + 1.0);
}
                        
[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, GROUP_SIZE_Z)]
void cshader(cshader_input input)
{
    g_histogram_shared[input.group_index] = 0;

    GroupMemoryBarrierWithGroupSync();

    if (input.dispatch_thread_id.x < input_dimensions.x &&
        input.dispatch_thread_id.y < input_dimensions.y)
    {
        float3 hdr_color = input_target.Load(int3(input.dispatch_thread_id.xy, 0)).rgb;
        uint bin_index = calculate_bin_index(hdr_color);
        InterlockedAdd(g_histogram_shared[bin_index], 1);
    }

    GroupMemoryBarrierWithGroupSync();

    histogram_buffer.InterlockedAdd(input.group_index * 4, g_histogram_shared[input.group_index]);
}  