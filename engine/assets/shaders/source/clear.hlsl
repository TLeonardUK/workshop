// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/fullscreen_pass.hlsl"
#include "data:shaders/source/common/gbuffer.hlsl"

gbuffer_output pshader(fullscreen_pinput input)
{
    gbuffer_fragment f;
    f.albedo = float3(0.5f, 0.5f, 0.5f);
    f.metallic = 0.0f;
    f.roughness = 0.0f;
    f.world_normal = float3(0.0f, 0.0f, 0.0f);
    f.world_position = float3(9999999.0f, 9999999.0f, 9999999.0f);
    f.flags = gbuffer_flag::none;

    return encode_gbuffer(f);
}