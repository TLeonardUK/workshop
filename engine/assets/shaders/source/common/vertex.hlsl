// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _VERTEX_HLSL_
#define _VERTEX_HLSL_

// Standard input to vertex shaders.
struct vertex_input
{
    uint vertex_id : SV_VertexID;
    uint instance_id : SV_InstanceID;
};

#endif