// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================

#include "data:shaders/source/common/vertex.hlsl"

struct imgui_pinput
{
    float4 position : SV_POSITION;
    float2 uv0 : TEXCOORD0;
    float4 color0 : TEXCOORD1;
};

struct imgui_poutput
{
    float4 color : SV_Target0;
};

imgui_pinput vshader(vertex_input input)
{
    vertex v = load_vertex(input.vertex_id);

    imgui_pinput result;
    result.position = mul(projection_matrix, float4(v.position.xy, 0.0, 1.0f));
    result.uv0 = v.uv0;
    result.color0 = v.color0;

    return result;
}

imgui_poutput pshader(imgui_pinput input)
{
    imgui_poutput f;
    f.color = color_texture.Sample(color_sampler, input.uv0) * input.color0;

    return f;
}