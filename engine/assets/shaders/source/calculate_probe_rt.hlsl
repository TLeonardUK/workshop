// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================

#include "data:shaders/source/common/vertex.hlsl"
#include "data:shaders/source/common/math.hlsl"
#include "data:shaders/source/common/sh.hlsl"

struct ray_payload
{
    float  hit_t;
    float3 world_position;
    int    instance_index;
    uint   volume_index;
    uint   instance_offset;
};

[shader("raygeneration")]
void ray_generation()
{
    // Load volume info

    // Generate a randomized ray direction

    // Trace the ray through the main scene tlas.    
    RayDesc ray;
    ray.Origin = float3(0.0f, 0.0f, 0.0f);
    ray.Direction = float3(0.0f, 1.0f, 0.0f);
    ray.TMin = 0.0f;
    ray.TMax = 1000.0f;
    
    ray_payload payload = (ray_payload)0;
    TraceRay(scene_tlas, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    
    // Ray missed
    if (payload.hit_t < 0.f)
    {
    }

    // Ray hit a surface backface

    // Calculate and store irradiance.

}

[shader("closesthit")]
void ray_closest_hit(inout ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    payload.hit_t = RayTCurrent();
    payload.world_position = WorldRayOrigin() + (WorldRayDirection() * payload.hit_t);

    payload.instance_index = (int)InstanceIndex();
    payload.volume_index = (InstanceID() >> 16);
    payload.instance_offset = (InstanceID() & 0xFFFF);

    // TODO: Sample all the albedo/normal;/metallic/roughness/etc parameters.
}

[shader("miss")]
void ray_miss(inout ray_payload payload)
{
    payload.hit_t = -1.f;
}