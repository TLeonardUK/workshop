// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _LIGHTING_HLSL_
#define _LIGHTING_HLSL_

#include "data:shaders/source/common/math.hlsl"

static const float3 dielectric_fresnel = 0.04;

enum light_type
{
    directional,
    point_,
    spotlight
};

float3 fresnel_schlick(float cos_theta, float3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

float distribution_ggx(float3 n, float3 h, float roughness)
{
    float a        = roughness * roughness;
    float a2       = a * a;
    float n_dot_h  = max(dot(n, h), 0.0);
    float n_dot_h2 = n_dot_h * n_dot_h;	
    float num      = a2;
    float denom    = (n_dot_h2 * (a2 - 1.0) + 1.0);

    denom = pi * denom * denom;
	
    return num / denom;
}

float geometry_schlick_ggx(float n_dot_v, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num   = n_dot_v;
    float denom = n_dot_v * (1.0 - k) + k;
	
    return num / denom;
}

float geometry_smith(float3 n, float3 v, float3 l, float roughness)
{
    float n_dot_v = max(dot(n, v), 0.0);
    float n_dot_l = max(dot(n, l), 0.0);
    float ggx2 = geometry_schlick_ggx(n_dot_v, roughness);
    float ggx1 = geometry_schlick_ggx(n_dot_l, roughness);
	
    return ggx1 * ggx2;
}

// http://www.humus.name/temp/Linearize%20depth.txt
float linear_eye_depth(float depth, float z_near, float z_far)
{
    float c1 = z_far / z_near;
    float c0 = 1.0 - c1;

    float t = 1.0 / (c0 * depth + c1);
    return t * z_far;
}

// Gets the index of the light cluster the given location 
// in screen space exists within.
uint get_light_cluster_index(float3 location, float2 tile_size_px, uint3 grid_size, float z_near, float z_far)
{
    float depth = linear_eye_depth(location.z, z_near, z_far);
    
    float scale = grid_size.z / log2(z_far / z_near);
    float bias = -(grid_size.z * log2(z_near) / log2(z_far / z_near));
    uint slice = uint(max(log2(depth) * scale + bias, 0.0));

    uint3 cluster = uint3(
            uint2(location.xy / tile_size_px), 
            slice 
    );

    uint index = cluster.x + 
                 (grid_size.x * cluster.y) +
                 (grid_size.x * grid_size.y) * cluster.z;

    return index;
}

#endif // _LIGHTING_HLSL_
