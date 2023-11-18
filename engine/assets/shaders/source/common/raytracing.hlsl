// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _RAYTRACING_HLSL_
#define _RAYTRACING_HLSL_

#include "data:shaders/source/common/math.hlsl"
//#include "data:shaders/source/common/raytracing_differentials.hlsl"

// Ensure these match the values in ri_types.h
enum ray_type
{
    primitive = 0,
    occlusion = 1,

    count
};

enum ray_mask
{
    ray_mask_normal = 1,
    ray_mask_sky = 2,
    ray_mask_invisible = 4,

    ray_mask_all = ray_mask_normal | ray_mask_sky | ray_mask_invisible,
    ray_mask_all_visible = ray_mask_normal | ray_mask_sky,
};

struct tlas_metadata_index
{
    uint table_index;
    uint table_offset;
};

struct ray_primitive_info
{
    vertex v1;
    vertex v2;
    vertex v3;

    uint v1_index;
    uint v2_index;
    uint v3_index;
};

tlas_metadata load_tlas_metadata()
{
    tlas_metadata_index index = scene_tlas_metadata.Load<tlas_metadata_index>(InstanceID() * sizeof(tlas_metadata_index));
    tlas_metadata meta = table_byte_buffers[index.table_index].Load<tlas_metadata>(index.table_offset);
    return meta;
}

material load_tlas_material(tlas_metadata metadata)
{
    return load_material_from_table(metadata.material_info_table, metadata.material_info_offset);
}

model_info load_tlas_model(tlas_metadata metadata)
{
    return table_byte_buffers[metadata.model_info_table].Load<model_info>(metadata.model_info_offset);
}

ray_primitive_info load_tlas_primitive(tlas_metadata metadata, model_info model)
{    
    ByteAddressBuffer index_buffer = table_byte_buffers[model.index_buffer_index];

    uint index0_offset = ((PrimitiveIndex() * 3) + 0) * model.index_size;
    uint index1_offset = ((PrimitiveIndex() * 3) + 1) * model.index_size;
    uint index2_offset = ((PrimitiveIndex() * 3) + 2) * model.index_size;

    uint index0_index = index_buffer.Load<uint>(index0_offset);
    uint index1_index = index_buffer.Load<uint>(index1_offset);
    uint index2_index = index_buffer.Load<uint>(index2_offset);

    ray_primitive_info ret;
    ret.v1_index = index0_index;
    ret.v2_index = index1_index;
    ret.v3_index = index2_index;
    ret.v1 = load_model_vertex(model, index0_index);
    ret.v2 = load_model_vertex(model, index1_index);
    ret.v3 = load_model_vertex(model, index2_index);
    return ret;
}

vertex interpolate_tlas_primitive(ray_primitive_info primitive, float2 barycentrics2)
{
    float3 barycentrics3 = float3(1 - barycentrics2.x - barycentrics2.y, barycentrics2.x, barycentrics2.y);

    vertex ret;
    ret.position = barycentric_lerp(primitive.v1.position, primitive.v2.position, primitive.v3.position, barycentrics3);
    ret.normal = normalize(barycentric_lerp(primitive.v1.normal, primitive.v2.normal, primitive.v3.normal, barycentrics3));
    ret.tangent = normalize(barycentric_lerp(primitive.v1.tangent, primitive.v2.tangent, primitive.v3.tangent, barycentrics3));
    ret.bitangent = normalize(barycentric_lerp(primitive.v1.bitangent, primitive.v2.bitangent, primitive.v3.bitangent, barycentrics3));
    ret.uv0 = barycentric_lerp(primitive.v1.uv0, primitive.v2.uv0, primitive.v3.uv0, barycentrics3);
    ret.uv1 = barycentric_lerp(primitive.v1.uv1, primitive.v2.uv1, primitive.v3.uv1, barycentrics3);
    ret.uv2 = barycentric_lerp(primitive.v1.uv2, primitive.v2.uv2, primitive.v3.uv2, barycentrics3);
    ret.uv3 = barycentric_lerp(primitive.v1.uv3, primitive.v2.uv3, primitive.v3.uv3, barycentrics3);
    ret.color0 = barycentric_lerp(primitive.v1.color0, primitive.v2.color0, primitive.v3.color0, barycentrics3);
    ret.color1 = barycentric_lerp(primitive.v1.color1, primitive.v2.color1, primitive.v3.color1, barycentrics3);
    ret.color2 = barycentric_lerp(primitive.v1.color2, primitive.v2.color2, primitive.v3.color2, barycentrics3);
    ret.color3 = barycentric_lerp(primitive.v1.color3, primitive.v2.color3, primitive.v3.color3, barycentrics3);
    return ret;
}

struct ray_shading_data
{
    tlas_metadata metadata;
    material mat;
    model_info model;    
    ray_primitive_info primitive;
    vertex params;
    float2 ddx;
    float2 ddy;
};

ray_shading_data load_ray_shading_data(float2 barycentrics)
{
    ray_shading_data ret;

    ret.metadata = load_tlas_metadata();
    ret.mat = load_tlas_material(ret.metadata);
    ret.model = load_tlas_model(ret.metadata);
    ret.primitive = load_tlas_primitive(ret.metadata, ret.model);
    ret.params = interpolate_tlas_primitive(ret.primitive, barycentrics);

    //compute_uv_differentials(ret);

    return ret;
}

#endif