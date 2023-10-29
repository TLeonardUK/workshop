// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _RAYTRACING_HLSL_
#define _RAYTRACING_HLSL_

// Ensure these match the values in ri_types.h
enum ray_type
{
    primitive = 0,
    occlusion = 1,

    count
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

ray_primitive_info load_tlas_primitive(tlas_metadata metadata)
{
    ray_primitive_info ret;

    // TODO: 
    //      - Load index buffer for verts.
    //      - Read vert data
    //      - Calculate uv based on barycentrics
    //      - Interpolate all values.

    return ret;
}

#endif