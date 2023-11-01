// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _RAYTRACING_PRIMITIVE_RAY_HLSL_
#define _RAYTRACING_PRIMITIVE_RAY_HLSL_

#include "data:shaders/source/common/gbuffer.hlsl"
#include "data:shaders/source/common/vertex.hlsl"
#include "data:shaders/source/common/normal.hlsl"
#include "data:shaders/source/common/lighting.hlsl"
#include "data:shaders/source/common/lighting_shading.hlsl"

struct primitive_ray_payload
{
    float3 color;
    float  opaque_hit_t;
    float  transparent_hit_t;
    float4 transparent_accumulation;
    float transparent_revealance;
};

[shader("miss")]
void ray_primitive_miss(inout primitive_ray_payload payload)
{
}

// ================================================================================================
// Opaque geometry
// ================================================================================================

float4 ray_primitive_common(inout primitive_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    ray_shading_data data = load_ray_shading_data(attrib.barycentrics);

    float4x3 object_to_world = ObjectToWorld4x3();
    float3 world_normal = mul(object_to_world, float4(data.params.normal, 0.0f));
    float3 world_tangent = mul(object_to_world, float4(data.params.tangent, 0.0f));
    float3 world_position = mul(object_to_world, float4(data.params.position, 1.0f));

    float4 albedo = data.mat.albedo_texture.SampleLevel(data.mat.albedo_sampler, data.params.uv0, 0);

    gbuffer_fragment f;
    f.albedo = albedo.rgb;
    f.flags = data.metadata.gpu_flags;
    f.metallic = data.mat.metallic_texture.SampleLevel(data.mat.metallic_sampler, data.params.uv0, 0).r;
    f.roughness = data.mat.roughness_texture.SampleLevel(data.mat.roughness_sampler, data.params.uv0, 0).r;
    f.world_normal = calculate_world_normal(
        unpack_compressed_normal(data.mat.normal_texture.SampleLevel(data.mat.normal_sampler, data.params.uv0, 0).xy),
        normalize(world_normal).xyz,
        normalize(world_tangent).xyz
    );
    f.world_position = world_position;
    f.uv = float2(0.0f, 0.0f);

    return float4(shade_fragment(f, true).rgb, albedo.a);
}

[shader("closesthit")]
void ray_primitive_opaque_closest_hit(inout primitive_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    payload.color = ray_primitive_common(payload, attrib).rgb;
    payload.opaque_hit_t = RayTCurrent();
}

// ================================================================================================
// Masked geometry
// ================================================================================================

[shader("closesthit")]
void ray_primitive_masked_closest_hit(inout primitive_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    payload.color = ray_primitive_common(payload, attrib).rgb;
    payload.opaque_hit_t = RayTCurrent();
}

[shader("anyhit")]
void ray_primitive_masked_any_hit(inout primitive_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
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
void ray_primitive_sky_closest_hit(inout primitive_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    tlas_metadata metadata = load_tlas_metadata();
    material mat = load_tlas_material(metadata);

    payload.color = mat.skybox_texture.SampleLevel(mat.skybox_sampler, WorldRayDirection(), 0.0f);
    payload.opaque_hit_t = RayTCurrent();
}

// ================================================================================================
// Transparent geometry
// ================================================================================================

[shader("anyhit")]
void ray_primitive_transparent_any_hit(inout primitive_ray_payload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    float4 shaded_color = ray_primitive_common(payload, attrib);
    float weight = calculate_wboit_weight(RayTCurrent(), shaded_color.a); 

    payload.transparent_accumulation += float4(shaded_color.rgb * shaded_color.a, shaded_color.a) * weight;
    payload.transparent_revealance += payload.transparent_revealance * (1.0 - shaded_color.a);
    payload.transparent_hit_t = payload.transparent_hit_t < 0.0f ? RayTCurrent() : min(payload.transparent_hit_t, RayTCurrent());

    IgnoreHit();
}

#endif