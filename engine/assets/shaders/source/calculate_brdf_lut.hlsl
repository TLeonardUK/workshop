// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/fullscreen_pass.hlsl"
#include "data:shaders/source/common/math.hlsl"
#include "data:shaders/source/common/lighting.hlsl"

struct pshader_output
{
    float4 color : SV_Target0;
};

float2 integrate_brdf(float n_dot_v, float roughness)
{
    float3 v;
    v.x = sqrt(1.0 - n_dot_v * n_dot_v);
    v.y = 0.0;
    v.z = n_dot_v;

    float a = 0.0;
    float b = 0.0;
    float3 n = float3(0.0, 0.0, 1.0);

    const uint SAMPLE_COUNT = 1024;
    for (uint i = 0; i < SAMPLE_COUNT; i++)
    {
        float2 xi = hammersley(i, SAMPLE_COUNT);
        float3 h  = importance_sample_ggx(xi, n, roughness);
        float3 l  = normalize(2.0 * dot(v, h) * h - v);

        float n_dot_l = max(l.z, 0.0);
        float n_dot_h = max(h.z, 0.0);
        float v_dot_h = max(dot(v, h), 0.0);

        if (n_dot_l > 0.0)
        {
            float g = geometry_smith_preintegrated(n, v, l, roughness);
            float g_vis = (g * v_dot_h) / (n_dot_h * n_dot_v);
            float fc = pow(1.0 - v_dot_h, 5.0);

            a += (1.0 - fc) * g_vis;
            b += fc * g_vis;
        }
    }

    a /= float(SAMPLE_COUNT);
    b /= float(SAMPLE_COUNT);

    return float2(a, b);
}

pshader_output pshader(fullscreen_pinput input)
{
    pshader_output output;
    output.color = float4(integrate_brdf(input.uv.x, input.uv.y), 0.0f, 1.0f);
    return output;
}