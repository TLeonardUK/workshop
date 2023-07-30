// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _CONSTS_HLSL_
#define _CONSTS_HLSL_

// This should match the visualization_mode enum in renderer.h
enum visualization_mode_t
{
    normal, 
    albedo,
    wireframe,
    metallic,
    roughness,
    world_normal,
    world_position,
    lighting,
    shadow_cascades,
    light_clusters,
    light_heatmap,
    light_probes
};

static const float3 debug_colors[8] = 
{
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, 0.0f, 1.0f),
    float3(1.0f, 0.0f, 0.0f),
    float3(0.0f, 1.0f, 1.0f),    
    float3(1.0f, 1.0f, 0.0f),
    float3(1.0f, 0.0f, 1.0f),
    float3(1.0f, 1.0f, 1.0f),
    float3(0.0f, 0.0f, 0.0f),
};

#endif