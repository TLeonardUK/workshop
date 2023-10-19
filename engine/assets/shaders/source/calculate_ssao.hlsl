// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/fullscreen_pass.hlsl"
#include "data:shaders/source/common/gbuffer.hlsl"
#include "data:shaders/source/common/normal.hlsl"
#include "data:shaders/source/common/tonemap.hlsl"
#include "data:shaders/source/common/consts.hlsl"
#include "data:shaders/source/common/lighting.hlsl"
#include "data:shaders/source/common/math.hlsl"

struct ssao_output
{
    float4 color : SV_Target0;
};

#if 0
static const int k_kernel_size = 64;
static const float3 k_kernel_samples[64] = {
    float3(0.0498, -0.0447, 0.0500),
    float3(0.0146, 0.0165, 0.0022),
    float3(-0.0406, -0.0194, 0.0319),
    float3(0.0138, -0.0916, 0.0409),
    float3(0.0560, 0.0598, 0.0577),
    float3(0.0923, 0.0443, 0.0155),
    float3(-0.0020, -0.0544, 0.0667),
    float3(-0.0003, -0.0002, 0.0004),
    float3(0.0500, -0.0466, 0.0254),
    float3(0.0381, 0.0314, 0.0329),
    float3(-0.0319, 0.0205, 0.0225),
    float3(0.0557, -0.0370, 0.0545),
    float3(0.0574, -0.0225, 0.0755),
    float3(-0.0161, -0.0038, 0.0555),
    float3(-0.0250, -0.0248, 0.0250),
    float3(-0.0337, 0.0214, 0.0254),
    float3(-0.0175, 0.0144, 0.0053),
    float3(0.0734, 0.1121, 0.0110),
    float3(-0.0441, -0.0903, 0.0837),
    float3(-0.0833, -0.0017, 0.0850),
    float3(-0.0104, -0.0329, 0.0193),
    float3(0.0032, -0.0049, 0.0042),
    float3(-0.0074, -0.0658, 0.0674),
    float3(0.0941, -0.0080, 0.1434),
    float3(0.0768, 0.1270, 0.1070),
    float3(0.0004, 0.0004, 0.0003),
    float3(-0.1048, 0.0654, 0.1017),
    float3(-0.0045, -0.1196, 0.1619),
    float3(-0.0746, 0.0344, 0.2241),
    float3(-0.0028, 0.0031, 0.0029),
    float3(-0.1085, 0.1423, 0.1664),
    float3(0.0469, 0.1036, 0.0596),
    float3(0.1346, -0.0225, 0.1305),
    float3(-0.1645, -0.1556, 0.1245),
    float3(-0.1877, -0.2088, 0.0578),
    float3(-0.0437, 0.0869, 0.0748),
    float3(-0.0026, -0.0020, 0.0041),
    float3(-0.0967, -0.1823, 0.2995),
    float3(-0.2258, 0.3161, 0.0892),
    float3(-0.0275, 0.2872, 0.3172),
    float3(0.2072, -0.2708, 0.1101),
    float3(0.0549, 0.1043, 0.3231),
    float3(-0.1309, 0.1193, 0.2802),
    float3(0.1540, -0.0654, 0.2298),
    float3(0.0529, -0.2279, 0.1485),
    float3(-0.1873, -0.0402, 0.0159),
    float3(0.1418, 0.0472, 0.1348),
    float3(-0.0443, 0.0556, 0.0559),
    float3(-0.0236, -0.0810, 0.2191),
    float3(-0.1421, 0.1981, 0.0052),
    float3(0.1586, 0.2305, 0.0437),
    float3(0.0300, 0.3818, 0.1638),
    float3(0.0830, -0.3097, 0.0674),
    float3(0.2270, -0.2354, 0.1937),
    float3(0.3813, 0.3320, 0.5295),
    float3(-0.5563, 0.2947, 0.3011),
    float3(0.4245, 0.0056, 0.1176),
    float3(0.3665, 0.0036, 0.0857),
    float3(0.3290, 0.0309, 0.1785),
    float3(-0.0829, 0.5128, 0.0566),
    float3(0.8674, -0.0027, 0.1001),
    float3(0.4557, -0.7720, 0.0038),
    float3(0.4173, -0.1548, 0.4625),
    float3(-0.4427, -0.6793, 0.1865)
};
#elif 1
static const int k_kernel_size = 32;
static const float3 k_kernel_samples[32] = {
    float3(0.0498, -0.0447, 0.0500),
    float3(0.0151, 0.0171, 0.0023),
    float3(-0.0460, -0.0219, 0.0361),
    float3(0.0178, -0.1182, 0.0528),
    float3(0.0845, 0.0903, 0.0870),
    float3(0.1643, 0.0789, 0.0275),
    float3(-0.0043, -0.1142, 0.1401),
    float3(-0.0008, -0.0005, 0.0009),
    float3(0.1426, -0.1329, 0.0723),
    float3(0.1245, 0.1026, 0.1074),
    float3(-0.1180, 0.0757, 0.0834),
    float3(0.2312, -0.1535, 0.2262),
    float3(0.2642, -0.1038, 0.3479),
    float3(-0.0814, -0.0191, 0.2808),
    float3(-0.1381, -0.1369, 0.1376),
    float3(-0.2009, 0.1275, 0.1515),
    float3(-0.1122, 0.0921, 0.0342),
    float3(0.5007, 0.7648, 0.0752),
    float3(-0.3189, -0.6535, 0.6057),
    float3(-0.6358, -0.0129, 0.6489),
    float3(-0.0834, -0.2635, 0.1545),
    float3(0.0269, -0.0409, 0.0349),
    float3(-0.0645, -0.5748, 0.5884),
    float3(0.8532, -0.0725, 1.2992),
    float3(0.7206, 1.1909, 1.0036),
    float3(0.0038, 0.0044, 0.0029),
    float3(-1.0442, 0.6521, 1.0138),
    float3(-0.0456, -1.2245, 1.6570),
    float3(-0.7821, 0.3614, 2.3514),
    float3(-0.0296, 0.0330, 0.0314),
    float3(-1.1895, 1.5603, 1.8245),
    float3(0.5241, 1.1586, 0.6660)
};
#else
static const int k_kernel_size = 16;
static const float3 k_kernel_samples[16] = {
float3(0.0498, -0.0447, 0.0500),
    float3(0.0151, 0.0171, 0.0023),
    float3(-0.0460, -0.0219, 0.0361),
    float3(0.0178, -0.1182, 0.0528),
    float3(0.0845, 0.0903, 0.0870),
    float3(0.1643, 0.0789, 0.0275),
    float3(-0.0043, -0.1142, 0.1401),
    float3(-0.0008, -0.0005, 0.0009),
    float3(0.1426, -0.1329, 0.0723),
    float3(0.1245, 0.1026, 0.1074),
    float3(-0.1180, 0.0757, 0.0834),
    float3(0.2312, -0.1535, 0.2262),
    float3(0.2642, -0.1038, 0.3479),
    float3(-0.0814, -0.0191, 0.2808),
    float3(-0.1381, -0.1369, 0.1376),
    float3(-0.2009, 0.1275, 0.1515)
};
#endif

ssao_output pshader(fullscreen_pinput input)
{
    float2 input_uv = (input.uv0 * uv_scale);

    gbuffer_fragment f = read_gbuffer(input_uv);

    float3 view_space_normal = normalize(mul(view_matrix, float4(f.world_normal, 0.0f))).xyz;
    float3 view_space_position = mul(view_matrix, float4(f.world_position, 1.0f)).xyz;
    
    float2 noise_scale = (view_dimensions * ssao_resolution_scale) / 4.0f;
    float2 noise_uv = input.uv0 * noise_scale;
    float3 random_vector = noise_texture.Sample(noise_texture_sampler, noise_uv).xyz;

    float3 view_space_tangent = normalize(random_vector - view_space_normal * dot(random_vector, view_space_normal));
    float3 view_space_bitangent = cross(view_space_normal, view_space_tangent);

    float3x3 tbn = float3x3(view_space_tangent, view_space_bitangent, view_space_normal);

    const float k_bias = 0.025f;
    const float k_max_uv_delta = (1.0f / view_dimensions.x) * ssao_radius * 10.0f;

    float ao = 0.0f;
    float sample_count = 0.0f;
    for (int i = 0; i < k_kernel_size; i++)
    {
        // Select a view space sample position from the random rotation kernel.
        float3 sample_pos = mul(tbn, k_kernel_samples[i]);
        sample_pos = sample_pos * ssao_radius + view_space_position;
         
        // Calculate clip space location of the fragment to sample.
        // Then convert it to a UV so we can actually sample it.
        float4 clip_space = mul(projection_matrix, float4(sample_pos, 1.0));
        clip_space.xy /= clip_space.w;
        clip_space.x = clip_space.x * 0.5f + 0.5f;          // 0...1 range
        clip_space.y = -clip_space.y * 0.5f + 0.5f;          // 0...1 range
        float2 view_uv = clip_space.xy;

        // Clamp distance sampled so we don't end up thrashing the texture cache by sampling very distance
        // uvs when close to geometry.
        float2 uv_offset_direct = (view_uv - input_uv);
        if (length(uv_offset_direct) > k_max_uv_delta)
        {
            continue;
        }

        gbuffer_fragment sample_frag = read_gbuffer(view_uv);
        float sample_depth = mul(view_matrix, float4(sample_frag.world_position, 1.0f)).z;
        
        float3 sample_dir = normalize(sample_pos - view_space_position);
        float n_dot_s = max(dot(view_space_normal, sample_dir), 0.0f);

        float range_check = smoothstep(0.0f, 1.0f, ssao_radius / abs(view_space_position.z - sample_depth));
        ao += range_check * (sample_depth + k_bias > sample_pos.z ? 0.0f : 1.0f) * n_dot_s;
        sample_count += 1.0f;
    }

    ao = 1.0f - (ao / sample_count);
    ao = saturate(pow(ao, ssao_power));

    ssao_output output;
    output.color = ao;
    return output;
}