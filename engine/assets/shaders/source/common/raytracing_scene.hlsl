// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _RAYTRACING_SCENE_HLSL_
#define _RAYTRACING_SCENE_HLSL_

#include "data:shaders/source/common/math.hlsl"

struct raytrace_scene_result
{
    float4 color;
    float  depth;
    bool   hit;
};

raytrace_scene_result raytrace_scene(RaytracingAccelerationStructure tlas, float3 origin, float3 direction, float max_distance)
{    
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0;
    ray.TMax = max_distance;

    // Initially find the closest depth, we use this to make sure we only blend transparent
    // triangles in-front of the closest opaque point.
    occlusion_ray_payload occlusion_payload;
    occlusion_payload.hit_t = -1.0f;

    TraceRay(
        tlas,
        RAY_FLAG_NONE,
        0xFF,
        ray_type::occlusion,
        ray_type::count,
        0,
        ray,
        occlusion_payload);

    if (occlusion_payload.hit_t == -1.0f)
    {
        raytrace_scene_result result;
        result.color = float4(0.0f, 0.0f, 0.0f, 0.0f);
        result.depth = 0.0f;
        result.hit = false;
        return result;
    }

    // Truncate ray to first opaque hit so we don't run the transparent anyhit shaders on occluded geometry.
    ray.TMax = occlusion_payload.hit_t + 0.001f;

    // Trace our actual shaded scene.
    primitive_ray_payload payload;
    payload.color = float3(1.0f, 1.0f, 1.0f); 
    payload.transparent_revealance = 1.0f;
    payload.transparent_accumulation = float4(0.0f, 0.0f, 0.0f, 0.0f);

    TraceRay(
        tlas,
        RAY_FLAG_NONE,
        0xFF,
        ray_type::primitive,
        ray_type::count,
        0,
        ray,
        payload);

    // Blend transparent and opaque hits so it matches raster pipeline.    
    float4 shaded_color = float4(payload.color.rgb, 1.0f);
    if (abs(payload.transparent_revealance - 1.0f) >= 0.001f)
    {
        float4 source = calculate_wboit_result(payload.transparent_accumulation, payload.transparent_revealance);
        shaded_color.rgb = (source.rgb * source.a) + (shaded_color.rgb * (1.0f - source.a));
    } 

    raytrace_scene_result result;
    result.color = shaded_color;
    result.depth = occlusion_payload.hit_t;
    result.hit = true;
    return result;
}

#endif