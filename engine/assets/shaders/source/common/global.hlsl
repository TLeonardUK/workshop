// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _GLOBAL_HLSL_
#define _GLOBAL_HLSL_

// This file is automatically included at the top of all shaders that are compiled
// in general this should only contain helper functionality used to support the shader compiler.

// Ensure this matches the definitions in common.yaml/in-code
struct vertex
{
    float3 position;
    float3 normal;
    float3 tangent;
    float3 bitangent;
    float2 uv0;
    float2 uv1;
    float2 uv2;
    float2 uv3;
    float4 color0;
    float4 color1;
    float4 color2;
    float4 color3;
};

vertex load_model_vertex(model_info info, uint vertex_id)
{
    vertex result = (vertex)0;

    // See k_vertex_stream_runtime_types in model.h for the types of each stream.

    if (info.position_buffer_index > 0)
    {
        result.position = table_byte_buffers[info.position_buffer_index].Load<float3>(vertex_id * sizeof(float3));
    }
    if (info.normal_buffer_index > 0)
    {
        result.normal = decompress_unit_vector(table_byte_buffers[info.normal_buffer_index].Load<float>(vertex_id * sizeof(float)));
    }
    if (info.tangent_buffer_index > 0)
    {
        result.tangent = decompress_unit_vector(table_byte_buffers[info.tangent_buffer_index].Load<float>(vertex_id * sizeof(float)));
    }
    if (info.bitangent_buffer_index > 0)
    {
        result.bitangent = decompress_unit_vector(table_byte_buffers[info.bitangent_buffer_index].Load<float>(vertex_id * sizeof(float)));
    }
    
    if (info.uv0_buffer_index > 0)
    {
        result.uv0 = table_byte_buffers[info.uv0_buffer_index].Load<float2>(vertex_id * sizeof(float2));
    }
    if (info.uv1_buffer_index > 0)
    {
        result.uv1 = table_byte_buffers[info.uv1_buffer_index].Load<float2>(vertex_id * sizeof(float2));
    }
    if (info.uv2_buffer_index > 0)
    {
        result.uv2 = table_byte_buffers[info.uv2_buffer_index].Load<float2>(vertex_id * sizeof(float2));
    }
    if (info.uv3_buffer_index > 0)
    {
        result.uv3 = table_byte_buffers[info.uv3_buffer_index].Load<float2>(vertex_id * sizeof(float2));
    }
    
    if (info.color0_buffer_index > 0)
    {
        result.color0 = table_byte_buffers[info.color0_buffer_index].Load<float4>(vertex_id * sizeof(float4));
    }
    if (info.color1_buffer_index > 0)
    {
        result.color1 = table_byte_buffers[info.color1_buffer_index].Load<float4>(vertex_id * sizeof(float4));
    }
    if (info.color2_buffer_index > 0)
    {
        result.color2 = table_byte_buffers[info.color2_buffer_index].Load<float4>(vertex_id * sizeof(float4));
    }
    if (info.color3_buffer_index > 0)
    {
        result.color3 = table_byte_buffers[info.color3_buffer_index].Load<float4>(vertex_id * sizeof(float4));
    }

    return result;
}

// Make sure this matches the enum in material.h
enum material_domain
{
    opaque,
    masked,
    transparent,
    sky
};

struct material
{
    material_domain domain;

    Texture2D albedo_texture;
    Texture2D opacity_texture;
    Texture2D metallic_texture;
    Texture2D roughness_texture;
    Texture2D normal_texture;
    TextureCube skybox_texture;

    sampler albedo_sampler;
    sampler opacity_sampler;
    sampler metallic_sampler;
    sampler roughness_sampler;
    sampler normal_sampler;
    sampler skybox_sampler;
};

material load_material_from_table(uint table, uint offset)
{
    material_info info = table_byte_buffers[table].Load<material_info>(offset);

    material mat;
    
    mat.domain = (material_domain)info.domain;

    mat.albedo_texture = table_texture_2d[info.albedo_texture_index];
    mat.opacity_texture = table_texture_2d[info.opacity_texture_index];
    mat.metallic_texture = table_texture_2d[info.metallic_texture_index];
    mat.roughness_texture = table_texture_2d[info.roughness_texture_index];
    mat.normal_texture = table_texture_2d[info.normal_texture_index];
    mat.skybox_texture = table_texture_cube[info.skybox_texture_index];

    mat.albedo_sampler = table_samplers[info.albedo_sampler_index];
    mat.opacity_sampler = table_samplers[info.opacity_sampler_index];
    mat.metallic_sampler = table_samplers[info.metallic_sampler_index];
    mat.roughness_sampler = table_samplers[info.roughness_sampler_index];
    mat.normal_sampler = table_samplers[info.normal_sampler_index];
    mat.skybox_sampler = table_samplers[info.skybox_sampler_index];

    return mat;
}

#endif