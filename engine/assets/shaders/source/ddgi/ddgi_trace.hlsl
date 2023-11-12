// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================

#include "data:shaders/source/common/vertex.hlsl"
#include "data:shaders/source/common/math.hlsl"
#include "data:shaders/source/common/quaternion.hlsl"
#include "data:shaders/source/common/octahedral.hlsl"
#include "data:shaders/source/common/sh.hlsl"
#include "data:shaders/source/common/lighting.hlsl"
#include "data:shaders/source/common/raytracing.hlsl"
#include "data:shaders/source/common/raytracing_primitive_ray.hlsl"
#include "data:shaders/source/common/raytracing_occlusion_ray.hlsl"
#include "data:shaders/source/common/raytracing_scene.hlsl"

float3 get_ray_direction(uint ray_index, uint ray_count)
{
    bool is_fixed = false;

    if (ray_index < PROBE_GRID_FIXED_RAY_COUNT)
    {
        is_fixed = true;
        ray_count = PROBE_GRID_FIXED_RAY_COUNT;
    }
    else
    {
        ray_index -= PROBE_GRID_FIXED_RAY_COUNT;
        ray_count -= PROBE_GRID_FIXED_RAY_COUNT;
    }

    // Get ray direction on a unit sphere.
    float3 ret = spherical_fibonacci(ray_index, ray_count);

    // If fixed don't rotate to keep results of relocation/etc temporarily stable.
    if (is_fixed)
    {
        return normalize(ret);
    }

    // Apply random rotation to ray.
    ret = normalize(quat_rotate(ret, quat_conjugate(random_ray_rotation)));

    return ret;
}

[shader("raygeneration")]
void ray_generation()
{
    uint launch_index = DispatchRaysIndex().x;
    uint probe_index = launch_index / probe_ray_count;
    uint probe_ray_index = launch_index % probe_ray_count;

    instance_offset_info info = probe_data_buffer.Load<instance_offset_info>(probe_index * sizeof(instance_offset_info));
    ddgi_probe_data probe_data = table_byte_buffers[NonUniformResourceIndex(info.data_buffer_index)].Load<ddgi_probe_data>(info.data_buffer_offset);

    light_probe_state probe_state = get_light_probe_state(probe_data.probe_state_buffer_index, probe_data.probe_index);

    // If not active, we only need to fire the fixed rays for relocation/classification.
    if (false)//probe_state.classification != (int)probe_classification::active && probe_ray_index >= PROBE_GRID_FIXED_RAY_COUNT)
    {
        ddgi_probe_scrach_data scratch;
        scratch.color = float3(0.0f, 0.0f, 0.0f);
        scratch.normal = float3(0.0f, 0.0f, 0.0f);
        scratch.distance = 0.0f;
        scratch.valid = false;
        scratch.back_face = false;
        
        scratch_buffer.Store<ddgi_probe_scrach_data>(launch_index * sizeof(ddgi_probe_scrach_data), scratch);

        return;
    }

    // Generate a puedo-random population of ray normals
    float3 ray_normal = get_ray_direction(probe_ray_index, probe_ray_count);

    // Trace scene and get result.
    raytrace_scene_result result = raytrace_scene(
        scene_tlas,
        probe_data.probe_origin + probe_state.position_offset, 
        ray_normal,
        probe_far_z
    );

    // Store result for later combining.
    ddgi_probe_scrach_data scratch;
    scratch.color = result.color;
    scratch.normal = ray_normal;
    scratch.distance = result.depth;
    scratch.valid = result.hit;
    scratch.back_face = (result.hit_kind == HIT_KIND_TRIANGLE_BACK_FACE);

    // Shorten distance to reduce influence during irradiance sampling, and invert
    // to track backface hits.
    if (scratch.back_face)
    {
        scratch.distance = -scratch.distance * 0.2f;
    }

    scratch_buffer.Store<ddgi_probe_scrach_data>(launch_index * sizeof(ddgi_probe_scrach_data), scratch);
} 
