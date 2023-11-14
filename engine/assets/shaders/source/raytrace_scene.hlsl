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
#include "data:shaders/source/common/raytracing_scene.hlsl"

#ifdef SHADER_STAGE_RAY_GENERATION
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

    // Trace scene and get result.
    raytrace_scene_result result = raytrace_scene(
        scene_tlas,
        ray_origin, 
        ray_direction,
        view_z_near + view_z_far
    );
    output_texture[launch_index] = result.color;
}
#endif