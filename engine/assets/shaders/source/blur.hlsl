// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/fullscreen_pass.hlsl"
#include "data:shaders/source/common/math.hlsl"
#include "data:shaders/source/common/lighting.hlsl"

struct pshader_output
{
    float4 color : SV_Target0;
};

// https://lisyarus.github.io/blog/graphics/2023/02/24/blur-coefficients-generator.html
// We should probably just calculate the kernel on the cpu and pass it in via uniforms rather than do this ...

#if BLUR_RADIUS==5
static const int k_sample_count = 5;
static const float k_kernel_offsets[5] = {
    -3.4048471718931532,
    -1.4588111840004858,
    0.48624268466894843,
    2.431625915613778,
    4
};
static const float k_kernel_weights[5] = {
    0.15642123799829394,
    0.26718801880015064,
    0.29738065394682034,
    0.21568339342709997,
    0.06332669582763516
};
#elif BLUR_RADIUS==10
static const int k_sample_count = 11;
static const float k_kernel_offsets[11] = {
    -9.260003189282239,
    -7.304547036499911,
    -5.353083811756559,
    -3.4048471718931532,
    -1.4588111840004858,
    0.48624268466894843,
    2.431625915613778,
    4.378621204796657,
    6.328357272092126,
    8.281739853232981,
    10
};
static const float k_kernel_weights[11] = {
    0.002071619848105582,
    0.012832728894200915,
    0.0517012035286156,
    0.1355841921107385,
    0.23159560769543552,
    0.2577662485651885,
    0.18695197035734282,
    0.08833722224378082,
    0.027179417353550506,
    0.005441161635553416,
    0.0005386277674878371
};
#else
#error "No kernel implemented for this blur size."
#endif

float4 blur(float2 blur_direction, float2 pixel_coord)
{
    float4 result = 0.0;
    float2 size = input_texture_size;

    for (int i = 0; i < k_sample_count; ++i)
    {
        float2 offset = blur_direction * k_kernel_offsets[i] / size;
        float weight = k_kernel_weights[i];
        result += input_texture.Sample(input_texture_sampler, pixel_coord + offset) * weight;
    }

    return result;
}

pshader_output pshader_horizontal(fullscreen_pinput input)
{
    pshader_output output;
    output.color = blur(float2(1, 0), input.uv0 * input_uv_scale);
    return output;
}

pshader_output pshader_vertical(fullscreen_pinput input)
{
    pshader_output output;
    output.color = blur(float2(0, 1), input.uv0 * input_uv_scale);
    return output;
}