// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================

#include "data:shaders/source/common/vertex.hlsl"
#include "data:shaders/source/common/math.hlsl"
#include "data:shaders/source/common/sh.hlsl"
#include "data:shaders/source/common/raytracing.hlsl"

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
void ray_primitive_miss(inout ray_payload payload)
{
    payload.hit_t = -1.f;
}

// Opaque geometry

[shader("closesthit")]
void ray_primitive_opaque_closest_hit(inout ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    tlas_metadata metadata = load_tlas_metadata();
    material mat = load_tlas_material(metadata);
    model_info model = load_tlas_model(metadata);

    // todo
}

// Masked geometry

[shader("closesthit")]
void ray_primitive_masked_closest_hit(inout ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    // todo
}

[shader("anyhit")]
void ray_primitive_masked_any_hit(inout ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    // todo
}

// Sky geometry

[shader("closesthit")]
void ray_primitive_sky_closest_hit(inout ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    // todo
}

// Transparent geometry

[shader("anyhit")]
void ray_primitive_transparent_any_hit(inout ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    // todo
}