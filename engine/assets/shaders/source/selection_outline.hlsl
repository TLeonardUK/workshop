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

float read_selection_flag(float2 uv)
{
    return (read_gbuffer(uv).flags & render_gpu_flags::selected) ? 1.0f : 0.0f;
}

float calculate_stength(float2 uv)
{
    float top_left      = read_selection_flag(uv + float2(-uv_step.x, -uv_step.y));
    float top           = read_selection_flag(uv + float2( 0.0f,      -uv_step.y));
    float top_right     = read_selection_flag(uv + float2( uv_step.x, -uv_step.y));
    float left          = read_selection_flag(uv + float2(-uv_step.x,  0.0f));
    float right         = read_selection_flag(uv + float2( uv_step.x,  0.0f));
    float bottom_left   = read_selection_flag(uv + float2(-uv_step.x,  uv_step.y));
    float bottom        = read_selection_flag(uv + float2( 0.0f,       uv_step.y));
    float bottom_right  = read_selection_flag(uv + float2( uv_step.x,  uv_step.y));
    float center        = read_selection_flag(uv + float2( 0.0f,       0.0f));

    // This is the standard sobel operator
    float x = top_left + 2.0 * left + bottom_left - top_right - 2.0 * right - bottom_right;
    float y = -top_left - 2.0 * top - top_right + bottom_left + 2.0 * bottom + bottom_right;

    float strength = sqrt((x * x) + (y * y));

    // Ensure all inner pixels have at least some contribution.
    strength = lerp(strength, fill_alpha, center);

    return strength;
}
 
swapchain_output pshader(fullscreen_pinput input)
{
    gbuffer_fragment f = read_gbuffer(input.uv);

    swapchain_output output;
    output.color = outline_color;
    output.color.a = calculate_stength(input.uv);
    return output;
}