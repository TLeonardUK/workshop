// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _OCTAHEDRAL_HLSL_
#define _OCTAHEDRAL_HLSL_

// Based on RTXGI

// Computes normalized octahedral coordinates for the given texel coordinates.
// Maps the top left texel to (-1,-1).
float2 get_normalized_octahedral_coordinates(int2 texCoords, int numTexels)
{
    // Map 2D texture coordinates to a normalized octahedral space
    float2 octahedralTexelCoord = float2(texCoords.x % numTexels, texCoords.y % numTexels);

    // Move to the center of a texel
    octahedralTexelCoord.xy += 0.5f;

    // Normalize
    octahedralTexelCoord.xy /= float(numTexels);

    // Shift to [-1, 1);
    octahedralTexelCoord *= 2.f;
    octahedralTexelCoord -= float2(1.f, 1.f);

    return octahedralTexelCoord;
}

// Computes the normalized octahedral direction that corresponds to the
// given normalized coordinates on the [-1, 1] square.
float3 get_octahedral_direction(float2 coords)
{
    float3 direction = float3(coords.x, coords.y, 1.f - abs(coords.x) - abs(coords.y));
    if (direction.z < 0.f)
    {
        direction.xy = (1.f - abs(direction.yx)) * sign_not_zero(direction.xy);
    }
    return normalize(direction);
}

// Computes the octant coordinates in the normalized [-1, 1] square, for the given a unit direction vector.
// The opposite of get_octahedral_direction().
float2 get_octahedral_coordinates(float3 direction)
{
    float l1norm = abs(direction.x) + abs(direction.y) + abs(direction.z);
    float2 uv = direction.xy * (1.f / l1norm);
    if (direction.z < 0.f)
    {
        uv = (1.f - abs(uv.yx)) * sign_not_zero(uv.xy);
    }
    return uv;
}
#endif