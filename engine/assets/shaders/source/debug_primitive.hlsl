// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================

#include "data:shaders/source/common/vertex.hlsl"
#include "data:shaders/source/common/tonemap.hlsl"

struct debug_primitive_pinput
{
    float4 position : SV_POSITION;
    float4 color0 : TEXCOORD1;
};

struct debug_primitive_poutput
{
    float4 color : SV_Target0;
};

debug_primitive_pinput vshader(vertex_input input)
{
    vertex v = load_vertex(input.vertex_id);

    float4x4 mvp_matrix = mul(projection_matrix, view_matrix);

    debug_primitive_pinput result;
    result.position = mul(mvp_matrix, float4(v.position, 1.0f));
    result.color0 = v.color0;

    return result;
}

debug_primitive_poutput pshader(debug_primitive_pinput input)
{
    debug_primitive_poutput f;
    f.color = input.color0;

    // Backbuffer is srgb, so cancel out the srgb colors passed in.
    f.color.rgb = srgb_to_linear(f.color.rgb);

    return f;
}