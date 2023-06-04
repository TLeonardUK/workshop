// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/common/vertex.hlsl"

struct fullscreen_pinput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

fullscreen_pinput fullscreen_vshader(vertex_input input)
{
    vertex v = load_vertex(input.vertex_id);

    fullscreen_pinput result;
    result.position = float4(v.position, 0.0f, 1.0f);
    result.uv = v.uv;

    return result;
}