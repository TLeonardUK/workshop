// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================

#include "data:shaders/source/common/vertex.hlsl"
#include "data:shaders/source/common/math.hlsl"
#include "data:shaders/source/common/sh.hlsl"
#include "data:shaders/source/common/lighting.hlsl"
#include "data:shaders/source/common/raytracing.hlsl"
#include "data:shaders/source/common/raytracing_primitive_ray.hlsl"
#include "data:shaders/source/common/raytracing_occlusion_ray.hlsl"

[shader("raygeneration")]
void ray_generation()
{
    float4x4 mvp_matrix = mul(projection_matrix, view_matrix);
    float3 forward_direction = mul(view_matrix, float4(0.0f, 0.0f, 1.0f, 0.0f));

    // Get location within output target
    uint2 launch_index = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions().xy);
    float2 ndc = (launch_index.xy + 0.5f) / dims.xy;
    ndc = float2(ndc.x, 1.0f - ndc.y);

    float3 ray_origin = ndc_to_world_space(ndc, inverse_projection_matrix, inverse_view_matrix).xyz;
    float3 ray_direction = (ray_origin - view_world_position);

    RayDesc ray;
    ray.Origin = ray_origin;
    ray.Direction = ray_direction;
    ray.TMin = 0;
    ray.TMax = view_z_near + view_z_far;

    // Initially find the closest depth, we use this to make sure we only blend transparent
    // triangles in-front of the closest opaque point.
    occlusion_ray_payload occlusion_payload;
    occlusion_payload.hit_t = -1.0f;

    TraceRay(
        scene_tlas,
        RAY_FLAG_CULL_NON_OPAQUE,
        0xFF,
        ray_type::occlusion,
        ray_type::count,
        0,
        ray,
        occlusion_payload);

    // Truncate ray to first opaque hit so we don't run the transparent anyhit shaders on occluded geometry.
    ray.TMax = occlusion_payload.hit_t + 0.001f;

    // Trace our actual shaded scene.
    primitive_ray_payload payload;
    payload.opaque_hit_t = -1.0f;
    payload.transparent_hit_t = -1.0f;
    payload.color = float3(1.0f, 0, 1.0f);
    payload.transparent_revealance = 1.0f;
    payload.transparent_accumulation = float4(0.0f, 0.0f, 0.0f, 0.0f);

    TraceRay(
        scene_tlas,
        RAY_FLAG_NONE,
        0xFF,
        ray_type::primitive,
        ray_type::count,
        0,
        ray,
        payload);

    // Blend transparent and opaque hits so it matches raster pipeline.    
    float4 output = float4(payload.color.rgb, 1.0f); ;
    if (payload.transparent_hit_t >= 0.0f)
    {
        float4 source = calculate_wboit_result(payload.transparent_accumulation, payload.transparent_revealance);
        output.rgb = (source.rgb * source.a) + (output.rgb * (1.0f - source.a));
    } 

    output_texture[launch_index] = output;
}
