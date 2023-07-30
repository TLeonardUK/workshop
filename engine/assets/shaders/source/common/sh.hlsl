// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _SH_HLSL_
#define _SH_HLSL_

#include "data:shaders/source/common/math.hlsl"

// Represents the coefficients for 3rd order spherical harmonics.
struct sh_coefficients
{
    float coefficients[9];
};

struct sh_color_coefficients
{
    float3 coefficients[9];
};

sh_coefficients calculate_sh_coefficients(float3 dir)
{
    sh_coefficients sh;

    // Order 1    
    sh.coefficients[0] = 0.282095f;

    // Order 2
    sh.coefficients[1] = 0.488603f * dir.y;
    sh.coefficients[2] = 0.488603f * dir.z;
    sh.coefficients[3] = 0.488603f * dir.x;

    // Order 3
    sh.coefficients[4] = 1.092548f * dir.x * dir.y;
    sh.coefficients[5] = 1.092548f * dir.y * dir.z;
    sh.coefficients[6] = 0.315392f * (3.0f * dir.z * dir.z - 1.0f);
    sh.coefficients[7] = 1.092548f * dir.x * dir.z;
    sh.coefficients[8] = 0.546274f * (dir.x * dir.x - dir.y * dir.y);

    return sh;
}

sh_coefficients calculate_sh_cosine_lobe(float3 dir)
{
    const float cosine_a0 = pi;
    const float cosine_a1 = (2.0f * pi) / 3.0f;
    const float cosine_a2 = pi * 0.25f;

    sh_coefficients sh;

    // Order 1    
    sh.coefficients[0] = 0.282095f * cosine_a0;

    // Order 2
    sh.coefficients[1] = 0.488603f * dir.y * cosine_a1;
    sh.coefficients[2] = 0.488603f * dir.z * cosine_a1;
    sh.coefficients[3] = 0.488603f * dir.x * cosine_a1;

    // Order 3
    sh.coefficients[4] = 1.092548f * dir.x * dir.y * cosine_a2;
    sh.coefficients[5] = 1.092548f * dir.y * dir.z * cosine_a2;
    sh.coefficients[6] = 0.315392f * (3.0f * dir.z * dir.z - 1.0f) * cosine_a2;
    sh.coefficients[7] = 1.092548f * dir.x * dir.z * cosine_a2;
    sh.coefficients[8] = 0.546274f * (dir.x * dir.x - dir.y * dir.y) * cosine_a2;

    return sh;
}

float3 calculate_sh_irradiance(float3 normal, sh_color_coefficients radiance)
{
    sh_coefficients sh_cosine = calculate_sh_cosine_lobe(normal);

    float3 irradiance = 0.0f;
    for (int i = 0; i < 9; i++)
    {
        irradiance += radiance.coefficients[i] * sh_cosine.coefficients[i];
    }

    return irradiance;
}

float3 calculate_sh_diffuse(float3 normal, sh_color_coefficients radiance, float3 albedo)
{
    return calculate_sh_irradiance(normal, radiance) * albedo / pi;
}

#endif