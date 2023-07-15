// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/fullscreen_pass.hlsl"
#include "data:shaders/source/common/gbuffer.hlsl"
#include "data:shaders/source/common/normal.hlsl"

struct swapchain_output
{
    float4 color : SV_Target0;
};

// This should match the visualization_mode enum in renderer.h
enum visualization_mode_t
{
    normal, 
    wireframe,
    metallic,
    roughness,
    world_normal,
    world_position,
    lighting
};

swapchain_output pshader(fullscreen_pinput input)
{
    gbuffer_fragment f = read_gbuffer(input.uv);

    swapchain_output output;

    switch (visualization_mode)
    {
        default:
        case visualization_mode_t::wireframe:
        case visualization_mode_t::normal:
        {
            output.color = float4(f.albedo, 1.0f);
            break;
        }
        case visualization_mode_t::metallic:
        {
            output.color = float4(f.metallic, 0.0f, 0.0f, 1.0f);
            break;
        }
        case visualization_mode_t::roughness:
        {
            output.color = float4(f.roughness, 0.0f, 0.0f, 1.0f);
            break;
        }
        case visualization_mode_t::world_normal:
        {
            output.color = float4(pack_normal(f.world_normal), 1.0f);
            break;
        }
        case visualization_mode_t::world_position:
        {
            output.color = float4(f.world_position, 1.0f);
            break;
        }
        case visualization_mode_t::lighting:
        {
            output.color = light_buffer_texture.Sample(light_buffer_sampler, input.uv);
            break;
        }
    }

    return output;
}