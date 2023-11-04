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

[shader("raygeneration")]
void ray_generation()
{
    uint launch_index = DispatchRaysIndex().x;

    // Generate a puedo-random population of ray normals
    float3 ray_normal = spherical_fibonacci(launch_index, probe_ray_count);

    // Trace scene and get result.
    raytrace_scene_result result = raytrace_scene(
        scene_tlas,
        probe_origin, 
        ray_normal,
        probe_far_z
    );

    // Store result for later combining.
    raytrace_diffuse_probe_scrach_data scratch;
    scratch.color = result.color;
    scratch.normal = ray_normal;
    scratch.valid = result.hit;
    scratch_buffer.Store<raytrace_diffuse_probe_scrach_data>(launch_index * sizeof(raytrace_diffuse_probe_scrach_data), scratch);
}

[numthreads(1, 1, 1)]
void combine_output_cshader()
{
    float3 coefficients[9];
    float hit_rays = 0.0f;

    // Zero out coefficients.
    for (int i = 0; i < 9; i++)
    {
        coefficients[i] = 0.0f;
    }

    // Accumulate the coefficients for each ray
    for (int i = 0; i < probe_ray_count; i++)
    {
        raytrace_diffuse_probe_scrach_data result = scratch_buffer.Load<raytrace_diffuse_probe_scrach_data>(i * sizeof(raytrace_diffuse_probe_scrach_data));
        if (result.valid)
        {
            sh_coefficients coeff = calculate_sh_coefficients(result.normal);
            for (int c = 0; c < 9; c++)
            {
                coefficients[c] += coeff.coefficients[c] * result.color; 
            }
            hit_rays += 1.0f;
        }
    }

    // Store resulting coefficients.
    float weight = 4.0 * pi;
    for (int c = 0; c < 9; c++)
    {
        float3 result = coefficients[c] * (weight / hit_rays);
        diffuse_output_buffer.Store(diffuse_output_buffer_start_offset + (c * sizeof(float3)), result);
    }
}