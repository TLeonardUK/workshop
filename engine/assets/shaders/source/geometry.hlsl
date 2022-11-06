// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/common.hlsl"

void Vertex(uint vertex_id : SV_VertexID)
{
    vertex v = load_vertex(vertex_id);
}

void Pixel()
{
}