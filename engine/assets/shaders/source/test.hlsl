// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/fullscreen_pass.hlsl"
#include "data:shaders/source/common/gbuffer.hlsl"

gbuffer_output pshader(fullscreen_pinput input)
{
    gbuffer_fragment f;
    f.albedo = albedo_texture.Sample(albedo_sampler, input.uv).rgb;
    f.metallic = 0.0f;
    f.roughness = 0.0f;
    f.world_normal = float3(0.0f, 0.0f, 0.0f);
    f.world_position = float3(0.0f, 0.0f, 0.0f);
    f.flags = 0;

    return encode_gbuffer(f);
}