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
};

[shader("raygeneration")]
void ray_generation()
{
    // todo
}

[shader("miss")]
void ray_miss(inout ray_payload payload)
{
    payload.hit_t = -1.f;
}

// Opaque geometry

[shader("closesthit")]
void ray_closest_hit_opaque(inout ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    // todo
}

// Masked geometry

[shader("closesthit")]
void ray_closest_hit_masked(inout ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    // todo
}

// Sky geometry

[shader("closesthit")]
void ray_closest_hit_sky(inout ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    // todo
}

// Transparent geometry

[shader("anyhit")]
void ray_any_hit_transparent(inout ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    // todo
}