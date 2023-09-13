// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _NORMAL_HLSL_
#define _NORMAL_HLSL_

float3 calculate_world_normal(
    float3 tangent_normal, 
    float3 world_normal, 
    float3 world_tangent)
{
    float3 n = world_normal;
    float3 t = world_tangent;
    float3 b = cross(n, t);
    float3x3 tbn = float3x3(t, b, n);

    return lerp(world_normal, mul(tangent_normal, tbn), 1.0f);
}

// Converts a normal's range from [0,1] to [-1,1]
float3 unpack_normal(float3 input)
{
    return (input * 2.0f) - 1.0f;
}

// Unpacks a normal that contains only the X and Y coordinates.
// Reconstructs the Z coordinator and scales to -1,1
float3 unpack_compressed_normal(float2 input)
{
    float2 scaled = (input * 2.0f) - 1.0f;
    float3 reconstructed = float3(
        scaled.x,
        -scaled.y, // We use opengl format for normals.
        sqrt(saturate(1.0f - dot(scaled, scaled)))
    );

    return reconstructed;
}

// Converts a normal's range from [-1,1] to [0,1]
float3 pack_normal(float3 input)
{
    return (input * 0.5f) + 0.5f;
}

#endif