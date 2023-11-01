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

float calculate_wboit_weight(float z, float a)
{
    return clamp(pow(min(1.0, a * 10.0) + 0.01, 3.0) * 1e8 * 
                     pow(1.0f - z * 0.9, 3.0), 1e-2, 3e3);
}

float4 calculate_wboit_result(float4 accumulation, float revelance)
{
    // Prevent overflow
    float max_val = max3(abs(accumulation.rgb));
    if (isinf(max_val))
    {
        accumulation.rgb = accumulation.a;
    }

    float3 average_color = accumulation.rgb / max(accumulation.a, 0.00001f);
    return float4(average_color.rgb, 1.0f - revelance);
}

float3 fresnel_schlick(float cos_theta, float3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

float3 fresnel_schlick_roughness(float cos_theta, float3 f0, float roughness)
{
    float3 r = 1.0 - roughness;
    return f0 + (max(r, f0) - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
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

float geometry_schlick_ggx_preintegrated(float n_dot_v, float roughness)
{
    float r = roughness;
    float k = (r * r) / 2.0;

    float num   = n_dot_v;
    float denom = n_dot_v * (1.0 - k) + k;
	
    return num / denom;
}

float geometry_smith_preintegrated(float3 n, float3 v, float3 l, float roughness)
{
    float n_dot_v = max(dot(n, v), 0.0);
    float n_dot_l = max(dot(n, l), 0.0);
    float ggx2 = geometry_schlick_ggx_preintegrated(n_dot_v, roughness);
    float ggx1 = geometry_schlick_ggx_preintegrated(n_dot_l, roughness);
	
    return ggx1 * ggx2;
}

float3 importance_sample_ggx(float2 xi, float3 n, float roughness)
{
    // This function is done at double precision as some of the numbers get teeny
    // and can give NaN's depending on how the instructions are ordered.
    // Fun fact, PIX's instrumentation adjusts the behaviour just enough to make
    // everything look like its running fine in the debugger. *flip table*

    double a = roughness * roughness;
	
    double phi = 2.0 * pi * xi.x;
    double cos_theta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    double sin_theta = sqrt(1.0 - cos_theta * cos_theta);
	
    // from spherical coordinates to cartesian coordinates
    double3 h;
    h.x = cos(phi) * sin_theta;
    h.y = sin(phi) * sin_theta;
    h.z = cos_theta;
	 
    // from tangent-space vector to world-space sample vector
    double3 up        = abs(n.z) < 0.999 ? double3(0.0, 0.0, 1.0) : double3(1.0, 0.0, 0.0);
    double3 tangent   = normalize(cross(up, n));
    double3 bitangent = cross(n, tangent);
	
    double3 sample_vec = tangent * h.x + bitangent * h.y + n * h.z;
    return normalize(sample_vec);
}  

float radical_inverse_vdc(uint bits) 
{
    // This is magic bullshit.
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 hammersley(uint i, uint n)
{
    return float2(float(i) / float(n), radical_inverse_vdc(i));
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
