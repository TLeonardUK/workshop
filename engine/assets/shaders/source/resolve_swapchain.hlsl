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

struct swapchain_output
{
    float4 color : SV_Target0;
};
 
swapchain_output pshader(fullscreen_pinput input)
{
    gbuffer_fragment f = read_gbuffer(input.uv);

    swapchain_output output;
    bool tonemap = false;

    switch (visualization_mode)
    {
        default:
        case visualization_mode_t::albedo:
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
        case visualization_mode_t::wireframe:
        case visualization_mode_t::normal:
        case visualization_mode_t::lighting:
        case visualization_mode_t::shadow_cascades:
        case visualization_mode_t::light_clusters:
        case visualization_mode_t::light_heatmap:        
        {
            output.color = light_buffer_texture.Sample(light_buffer_sampler, input.uv);
            tonemap = true;
            break;
        }
    }
    
    // Gamma correct the output
    if (tonemap)
    {
        output.color.rgb = tonemap_reinhard(output.color.rgb);
    }

    return output;
}