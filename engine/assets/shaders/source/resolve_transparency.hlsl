// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/fullscreen_pass.hlsl"
#include "data:shaders/source/common/lighting.hlsl"

// Reference:
//  https://jcgt.org/published/0002/02/09/paper.pdf

struct pshader_output
{
    float4 color : SV_Target0;
};

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

    pshader_output output;
    output.color = calculate_wboit_result(accumulation, revelance);
    return output;
}