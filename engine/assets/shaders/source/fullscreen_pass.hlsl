// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================

struct fullscreen_pinput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

fullscreen_pinput fullscreen_vshader(uint vertex_id : SV_VertexID)
{
    vertex v = load_vertex(vertex_id);

    fullscreen_pinput result;
    result.position = float4(v.position, 0.0f, 1.0f);
    result.uv = v.uv;

    return result;
}