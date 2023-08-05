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
                        
[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, GROUP_SIZE_Z)]
void cshader(cshader_input input)
{
    uint bin_count = histogram_buffer.Load<uint>(input.group_index * 4);
    g_histogram_shared[input.group_index] = bin_count * input.group_index;

    GroupMemoryBarrierWithGroupSync();

    histogram_buffer.Store(input.group_index * 4, 0);    

    [unroll]
    for (uint sample_index = (HISTOGRAM_SIZE >> 1); sample_index > 0; sample_index >>= 1)
    {
        if (input.group_index < sample_index)
        {
            g_histogram_shared[input.group_index] += g_histogram_shared[input.group_index + sample_index];
        }
        
        GroupMemoryBarrierWithGroupSync();
    }

    if (input.group_index == 0)
    {
        float bin_value = g_histogram_shared[0];
        uint pixel_count = (input_dimensions.x * input_dimensions.y);

        float weighted_log_average = (bin_value.x / max((float)pixel_count - bin_count, 1.0)) - 1.0;

        float weighted_average_luminance = exp2(((weighted_log_average / 254.0) * log2_luminance_range) + min_log2_luminance);

        float luminance_last_frame = average_buffer.Load<float>(0);
        float adapted_luminance = luminance_last_frame + (weighted_average_luminance - luminance_last_frame) * time_coeff;
        average_buffer.Store(0, adapted_luminance);
    }
}  