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
    f.opacity = 1.0f;
    f.metallic = 0.0f;
    f.roughness = 0.0f;
    f.world_normal = float3(0.0f, 0.0f, 0.0f);
    f.world_position = float3(0.0f, 0.0f, 0.0f);

    return encode_gbuffer(f);
}