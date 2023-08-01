// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================

#include "data:shaders/source/common/vertex.hlsl"
#include "data:shaders/source/common/math.hlsl"
#include "data:shaders/source/common/sh.hlsl"

// Some useful reading:
// https://github.com/nicknikolov/cubemap-sh/blob/master/index.js
// http://www.ppsloan.org/publications/StupidSH36.pdf
// https://www.gamedev.net/forums/topic/671562-spherical-harmonics-cubemap/
// https://github.com/microsoft/DirectXMath/blob/22e6d747994600e00834faff5fc2a95ab60f1790/SHMath/DirectXSHD3D12.cpp#L182

struct cshader_input
{
	uint3 group_thread_id : SV_GroupThreadID;
	uint3 group_id : SV_GroupID;
	uint3 dispatch_thread_id : SV_DispatchThreadID;
	uint group_index : SV_GroupIndex;
};

float get_texel_weight(int x, int y, int size)
{
    float fB = -1.0f + 1.0f / size;
    float fS = 2.0f * (1.0f - 1.0f / size) / (size - 1.0f);

    float u = float(x) * fS + fB;
    float v = float(y) * fS + fB;

    float temp = 1.0f + u * u + v * v;
    return 4.0f / (temp * sqrt(temp));
}

groupshared float3 shared_coefficients[6][9];
groupshared float3 shared_weights[6];

[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, GROUP_SIZE_Z)]
void cshader(cshader_input input)
{
    int face_index = input.group_thread_id.z;
    float weight_accumulation = 0.0f;
    float3 coefficients[9];

    // Zero out coefficients.
    for (int i = 0; i < 9; i++)
    {
        coefficients[i] = 0.0f;
    }

    // Accumulate the coefficients for each texel.
    for (int y = 0; y < cube_size; y++)
    {
        for (int x = 0; x < cube_size; x++)
        {
            float3 texel_normal = get_cubemap_normal(face_index, cube_size, int2(x, y));
            float3 texel_color = cube_texture.SampleLevel(cube_sampler, texel_normal, 0);

            float weight = get_texel_weight(x, y, cube_size);
            weight_accumulation += weight;

            sh_coefficients coeff = calculate_sh_coefficients(texel_normal);
            for (int c = 0; c < 9; c++)
            {
                coefficients[c] += coeff.coefficients[c] * texel_color * weight; 
            }
        }
    }

    // Store in sharec memory for combination.
    for (int i = 0; i < 9; i++)
    {
        shared_coefficients[face_index][i] = coefficients[i];
        shared_weights[face_index] = weight_accumulation;
    }

    GroupMemoryBarrierWithGroupSync();

    // Combine coefficients.
    if (input.group_thread_id.z == 0)
    {
        // Combine and store coefficients for each face.   
        for (int j = 0; j < 9; j++)
        {
            weight_accumulation = 0.0f;
            float3 result = 0.0f;

            for (int i = 0; i < 6; i++)
            {
                result += shared_coefficients[i][j];
                weight_accumulation += shared_weights[i];
            }

            float normal_projection = (4.0f * pi) / weight_accumulation;
            result = result * normal_projection;  
            
            output_buffer.Store(output_start_offset + (j * sizeof(float3)), result);
        }
    }
}