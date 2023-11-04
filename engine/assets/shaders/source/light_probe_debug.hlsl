// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/common/vertex.hlsl"
#include "data:shaders/source/common/sh.hlsl"

struct light_probe_pinput
{
    float4 position : SV_POSITION;
    float4 world_normal : NORMAL1;
    int3 probe_location : TEXCOORD3;
};

struct light_probe_poutput
{
    float4 color : SV_Target0;
};

light_probe_pinput vshader(vertex_input input)
{
    vertex v = load_vertex(input.vertex_id);
    light_probe_instance_info vi = load_light_probe_instance_info(input.instance_id);

    float4x4 mvp_matrix = mul(projection_matrix, mul(view_matrix, vi.model_matrix));

    light_probe_pinput result;
    result.position = mul(mvp_matrix, float4(v.position, 1.0f));
    result.world_normal = mul(vi.model_matrix, float4(v.normal, 0.0f));
    result.probe_location = uint3(vi.is_valid, vi.sh_table_index, vi.sh_table_offset);

    return result;
}

light_probe_poutput pshader(light_probe_pinput input)
{
    light_probe_poutput output;

    if (!input.probe_location.x)
    {
        output.color = float4(1.0f, 0.0f, 0.0f, 1.0f);
    }
    else
    {
        float3 sample_normal = normalize(input.world_normal).xyz;

        RWByteAddressBuffer sh_buffer = table_rw_byte_buffers[NonUniformResourceIndex(input.probe_location.y)];
        sh_color_coefficients radiance = sh_buffer.Load<sh_color_coefficients>(input.probe_location.z);

        float3 diffuse = calculate_sh_diffuse(sample_normal, radiance, float3(1.0f, 1.0f, 1.0f));
        output.color = float4(diffuse, 1.0f);
    }

    return output;
}
