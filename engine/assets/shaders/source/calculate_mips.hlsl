// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/common/math.hlsl"

struct cshader_input
{
	uint3 dispatch_thread_id : SV_DispatchThreadID;
};

[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, GROUP_SIZE_Z)]
void cshader(cshader_input input)
{
    float2 tex_coords = texel_size * (input.dispatch_thread_id.xy + 0.5);
    float4 color = source_texture.SampleLevel(source_sampler, tex_coords, 0);
    dest_texture[input.dispatch_thread_id.xy] = color;   
}  