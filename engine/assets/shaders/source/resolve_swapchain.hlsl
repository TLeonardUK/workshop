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
    gbuffer_fragment f = read_gbuffer(input.uv * uv_scale);

    swapchain_output output;
    bool tonemap = false;

    switch (visualization_mode)
    {
        default:
        case visualization_mode_t::albedo:
        case visualization_mode_t::metallic:
        case visualization_mode_t::roughness:
        case visualization_mode_t::world_normal:
        case visualization_mode_t::world_position:
        case visualization_mode_t::lighting:
        case visualization_mode_t::shadow_cascades:
        case visualization_mode_t::light_clusters:
        case visualization_mode_t::light_heatmap:  
        case visualization_mode_t::light_probes:    
        case visualization_mode_t::light_probe_contribution:             
        case visualization_mode_t::indirect_specular:
        case visualization_mode_t::indirect_diffuse:    
        case visualization_mode_t::direct_light:
        case visualization_mode_t::wireframe:
        case visualization_mode_t::ao:
        {
            tonemap = false;
            break;
        }
        case visualization_mode_t::normal:
        {
            tonemap = true;
            break;
        }
    }

    // Grab fragment color from lit buffer.
    output.color = light_buffer_texture.Sample(light_buffer_sampler, input.uv * uv_scale);

    // Gamma correct the output
    if (tonemap && tonemap_enabled)
    {
        // Load the average luminance.
        float average_luminance = average_luminance_buffer.Load<float>(0);
        float3 color = output.color.rgb;

        // https://bruop.github.io/exposure/
        float3 yxy = convert_rgb_to_yxy(color);
        yxy.x /= (9.6f * average_luminance + 0.0001);
        color = convert_yxy_to_rgb(yxy);

        output.color.rgb = tonemap_reinhard2(color, white_point_squared);
    }

    return output;
}