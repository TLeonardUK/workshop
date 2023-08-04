// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/common/gbuffer.hlsl"
#include "data:shaders/source/common/vertex.hlsl"
#include "data:shaders/source/common/normal.hlsl"
#include "data:shaders/source/common/lighting.hlsl"

struct geometry_pinput
{
    float4 position : SV_POSITION;
    float4 world_normal : TEXCOORD1;
};

geometry_pinput vshader(vertex_input input)
{
    vertex v = load_vertex(input.vertex_id);
    geometry_instance_info vi = load_geometry_instance_info(input.instance_id);

    float4x4 mvp_matrix = mul(projection_matrix, view_matrix);

    float3 world_position = mul(vi.model_matrix, float4(v.position, 1.0f)) + view_world_position;

    geometry_pinput result;
    result.position = mul(mvp_matrix, float4(world_position, 1.0f));
    result.world_normal = mul(vi.model_matrix, float4(v.normal, 0.0f));

    return result;
}

gbuffer_output_with_depth pshader(geometry_pinput input)
{
    float4 albedo = skybox_texture.Sample(skybox_sampler, -input.world_normal);

    gbuffer_fragment f;
    f.albedo = albedo.rgb;
    f.metallic = 0.0f;
    f.roughness = 0.0f;
    f.world_normal = float3(0.0f, 0.0f, 0.0f);
    f.world_position = float3(0.0f, 0.0f, 0.0f);
    f.flags = gbuffer_flag::unlit;
    
    gbuffer_output encoded_gbuffer = encode_gbuffer(f);
    gbuffer_output_with_depth output;
    output.buffer0 = encoded_gbuffer.buffer0;
    output.buffer1 = encoded_gbuffer.buffer1;
    output.buffer2 = encoded_gbuffer.buffer2;
    output.depth = 0.9999999f;

    return output;
}
