// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/fullscreen_pass.hlsl"
#include "data:shaders/source/common/gbuffer.hlsl"

struct swapchain_output
{
    float4 color : SV_Target0;
};

swapchain_output pshader(fullscreen_pinput input)
{
    gbuffer_fragment f = read_gbuffer(input.uv);

    swapchain_output output;
    output.color = float4(f.albedo, 1.0f);

    return output;
}