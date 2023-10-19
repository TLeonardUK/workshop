// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/fullscreen_pass.hlsl"
#include "data:shaders/source/common/gbuffer.hlsl"
#include "data:shaders/source/common/lighting_shading.hlsl"

struct lightbuffer_output
{
    float4 color : SV_Target0;
};

lightbuffer_output pshader(fullscreen_pinput input)
{
    gbuffer_fragment frag = read_gbuffer(input.uv0 * uv_scale);

    lightbuffer_output output;
    output.color = shade_fragment(frag, false);
    return output;
}