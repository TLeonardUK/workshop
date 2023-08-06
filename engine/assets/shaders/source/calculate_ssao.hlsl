// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/fullscreen_pass.hlsl"
#include "data:shaders/source/common/gbuffer.hlsl"
#include "data:shaders/source/common/normal.hlsl"
#include "data:shaders/source/common/tonemap.hlsl"
#include "data:shaders/source/common/consts.hlsl"
#include "data:shaders/source/common/lighting.hlsl"

struct ssao_output
{
    float4 color : SV_Target0;
};
 
ssao_output pshader(fullscreen_pinput input)
{
    gbuffer_fragment f = read_gbuffer(input.uv * uv_scale);

    float ao = 0.1f; // TODO

    ssao_output output;
    output.color = ao;
    return output;
}