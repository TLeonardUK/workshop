// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================

float3 calculate_world_normal(
    float3 tangent_normal_unnorm, 
    float3 world_normal, 
    float3 world_tangent,
    float3 world_bitangent)
{
    // [0,1] to [-1,1]
    // huuuuuuum, why does the - give is roughly what we want, where 
    // is this inversion coming from ....

    // Has to be something to do with the format of the texture right? The values
    // are so close to the default there shouldn't be any reason for it to change ...
    float3 tangent_normal = normalize((tangent_normal_unnorm * 2.0f) - 1.0f);
    float3 n = world_normal;
    float3 t = world_tangent;
    float3 b = world_bitangent;
    float3x3 tbn = float3x3(t, b, n);

    return mul(tangent_normal, tbn);
}