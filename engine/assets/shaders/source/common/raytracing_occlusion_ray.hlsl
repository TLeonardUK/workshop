// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _RAYTRACING_OCCLUSION_RAY_HLSL_
#define _RAYTRACING_OCCLUSION_RAY_HLSL_

struct occlusion_ray_payload
{
    float  hit_t;
};

[shader("miss")]
void ray_occlusion_miss(inout occlusion_ray_payload payload)
{
}

// Opaque geometry

[shader("closesthit")]
void ray_occlusion_opaque_closest_hit(inout occlusion_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
}

// Masked geometry

[shader("closesthit")]
void ray_occlusion_masked_closest_hit(inout occlusion_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
}

[shader("anyhit")]
void ray_occlusion_masked_any_hit(inout occlusion_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
}

// Sky geometry

[shader("closesthit")]
void ray_occlusion_sky_closest_hit(inout occlusion_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
}

// Transparent geometry

[shader("anyhit")]
void ray_occlusion_transparent_any_hit(inout occlusion_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
}

#endif