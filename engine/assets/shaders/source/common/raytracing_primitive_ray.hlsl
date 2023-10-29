// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _RAYTRACING_PRIMITIVE_RAY_HLSL_
#define _RAYTRACING_PRIMITIVE_RAY_HLSL_

struct primitive_ray_payload
{
    float  hit_t;
    float3 color;
};

[shader("miss")]
void ray_primitive_miss(inout primitive_ray_payload payload)
{
    payload.hit_t = -1.0f;
    payload.color = float3(1.0f, 1.0f, 0.0f);
}

// Opaque geometry

[shader("closesthit")]
void ray_primitive_opaque_closest_hit(inout primitive_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    //tlas_metadata metadata = load_tlas_metadata();
    //material mat = load_tlas_material(metadata);
    //model_info model = load_tlas_model(metadata);
    //ray_primitive_info primitive = load_tlas_primitive(metadata);

    // TODO: Calculate shaded value.

    payload.color = float3(1.0f, 1.0f, 1.0f);
}

// Masked geometry

[shader("closesthit")]
void ray_primitive_masked_closest_hit(inout primitive_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    // todo
    payload.color = float3(1.0f, 0.0f, 0.0f);
}

[shader("anyhit")]
void ray_primitive_masked_any_hit(inout primitive_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    // todo
}

// Sky geometry

[shader("closesthit")]
void ray_primitive_sky_closest_hit(inout primitive_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    tlas_metadata metadata = load_tlas_metadata();
    material mat = load_tlas_material(metadata);

    payload.color = mat.skybox_texture.SampleLevel(mat.skybox_sampler, WorldRayDirection(), 0.0f);
}

// Transparent geometry

[shader("anyhit")]
void ray_primitive_transparent_any_hit(inout primitive_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    // todo
    payload.color = float3(0.0f, 1.0f, 0.0f);
}

#endif