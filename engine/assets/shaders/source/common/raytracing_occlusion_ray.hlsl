// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _RAYTRACING_OCCLUSION_RAY_HLSL_
#define _RAYTRACING_OCCLUSION_RAY_HLSL_

#include "data:shaders/source/common/gbuffer.hlsl"
#include "data:shaders/source/common/vertex.hlsl"
#include "data:shaders/source/common/normal.hlsl"
#include "data:shaders/source/common/lighting.hlsl"
#include "data:shaders/source/common/lighting_shading.hlsl"

struct occlusion_ray_payload
{
    float  hit_t;
};

[shader("miss")]
void ray_occlusion_miss(inout occlusion_ray_payload payload)
{
    payload.hit_t = -1.0f;
}

// ================================================================================================
// Opaque geometry
// ================================================================================================

[shader("closesthit")]
void ray_occlusion_opaque_closest_hit(inout occlusion_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    payload.hit_t = RayTCurrent();
}

// ================================================================================================
// Masked geometry
// ================================================================================================

[shader("closesthit")]
void ray_occlusion_masked_closest_hit(inout occlusion_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    payload.hit_t = RayTCurrent();
}

[shader("anyhit")]
void ray_occlusion_masked_any_hit(inout occlusion_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    ray_shading_data data = load_ray_shading_data(attrib.barycentrics);
    float alpha = data.mat.albedo_texture.SampleLevel(data.mat.albedo_sampler, data.params.uv0, 0).a;

    if (alpha <= 0.0f)
    {
        IgnoreHit();
    }
    else
    {
        AcceptHitAndEndSearch();
    }
}

// ================================================================================================
// Sky geometry
// ================================================================================================

[shader("closesthit")]
void ray_occlusion_sky_closest_hit(inout occlusion_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    payload.hit_t = RayTCurrent();
}

// ================================================================================================
// Transparent geometry
// ================================================================================================

[shader("anyhit")]
void ray_occlusion_transparent_any_hit(inout occlusion_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    IgnoreHit();
}

#endif