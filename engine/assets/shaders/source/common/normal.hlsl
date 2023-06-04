// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================

float3 calculate_world_normal(float3 normal_map_sample, float3 surface_world_normal, float3 tangent)
{
    float3 local_normal = (normal_map_sample * 0.5) + 0.5;
    float3 n = surface_world_normal;
    float3 t = normalize(tangent - dot(tangent, n) * n); 
    float3 b = cross(n, t);
    float3x3 tbn = float3x3(t, b, n);

    return mul(local_normal, tbn);
}