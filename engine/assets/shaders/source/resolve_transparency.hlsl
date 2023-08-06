// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/fullscreen_pass.hlsl"

// Reference:
//  https://jcgt.org/published/0002/02/09/paper.pdf

struct pshader_output
{
    float4 color : SV_Target0;
};

float3 max3(float3 input)
{
    return (max(input.x, input.y), input.z);
}
 
pshader_output pshader(fullscreen_pinput input)
{
    int3 coords = int3(input.position.xy, 0);

    float revelance = revealance_texture.Load(coords).r;

    // No transparent fragment at this texel so discard.
    if (abs(revelance - 1.0f) < 0.00001f)
    {
        discard;
    }

    float4 accumulation = accumulation_texture.Load(coords);

    // Prevent overflow
    float max_val = max3(abs(accumulation.rgb));
    if (isinf(max_val))
    {
        accumulation.rgb = accumulation.a;
    }

    float3 average_color = accumulation.rgb / max(accumulation.a, 0.00001f);

    pshader_output output;
    output.color = float4(average_color.rgb, 1.0f - revelance);
    return output;
}