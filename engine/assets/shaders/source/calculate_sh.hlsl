// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================

#include "data:shaders/source/common/vertex.hlsl"
#include "data:shaders/source/common/math.hlsl"

// Some useful reading:
// https://github.com/nicknikolov/cubemap-sh/blob/master/index.js
// http://www.ppsloan.org/publications/StupidSH36.pdf
// https://www.gamedev.net/forums/topic/671562-spherical-harmonics-cubemap/

struct cshader_input
{
	uint3 group_thread_id : SV_GroupThreadID;
	uint3 group_id : SV_GroupID;
	uint3 dispatch_thread_id : SV_DispatchThreadID;
	uint group_index : SV_GroupIndex;
};

float get_texel_area(float x, float y)
{
    return atan2(x * y, sqrt(x * x + y * y + 1.0));
}

float get_texel_solid_angle(int x, int y, int width, int height)
{
    float2 uv = float2(
        (2.0 * (x + 0.5) / width) - 1.0,
        (2.0 * (y + 0.5) / height) - 1.0
    );

    float2 inverse_size = float2(
        1.0f / width,
        1.0f / height
    );

    float x0 = uv.x - inverse_size.x;
    float y0 = uv.y - inverse_size.y;
    float x1 = uv.x + inverse_size.x;
    float y1 = uv.y + inverse_size.y;

    float angle = get_texel_area(x0, y0) - get_texel_area(x0, y1) - get_texel_area(x1, y0) + get_texel_area(x1, y1);

    return angle;
}

groupshared float3 shared_coefficients[6][9];

[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, GROUP_SIZE_Z)]
void cshader(cshader_input input)
{
    int face_index = input.group_thread_id.z;
    float weight_accumulation = 0.0f;
    float3 coefficients[9];

    // Go over each texel in cubemap and integrate it.
    float2 uv_step = 1.0f / cube_size;

    // Zero out coefficients.
    for (int i = 0; i < 9; i++)
    {
        coefficients[i] = 0.0f;
    }

    // Accumulate the coefficients.
    for (int y = 0; y < cube_size; y++)
    {
        for (int x = 0; x < cube_size; x++)
        {
            float weight = get_texel_solid_angle(x, y, cube_size, cube_size);
            float weight_1 = weight * 4.0f / 17.0f;
            float weight_2 = weight * 8.0f / 17.0f;
            float weight_3 = weight * 15.0f / 17.0f;
            float weight_4 = weight * 5.0f / 68.0f;
            float weight_5 = weight * 15.0f / 68.0f;
            
            float3 texel_normal = get_cubemap_normal(face_index, float2(x, y) * uv_step);
            float3 texel_color = cube_texture.SampleLevel(cube_sampler, texel_normal, 0);

            // order 1
            coefficients[0] += texel_color * weight_1;

            // order 2
            coefficients[1] += texel_color * weight_2 * texel_normal.x;
            coefficients[2] += texel_color * weight_2 * texel_normal.y;
            coefficients[3] += texel_color * weight_2 * texel_normal.z;

            // order 3
            coefficients[4] += texel_color * weight_3 * texel_normal.x * texel_normal.z;
            coefficients[5] += texel_color * weight_3 * texel_normal.z * texel_normal.y;
            coefficients[6] += texel_color * weight_3 * texel_normal.y * texel_normal.x;
            coefficients[7] += texel_color * weight_4 * (3.0f * texel_normal.z * texel_normal.z - 1.0f);
            coefficients[8] += texel_color * weight_5 * (texel_normal.x * texel_normal.x - texel_normal.y * texel_normal.y);

            weight_accumulation += weight;
        }
    }

    // Normalize and correct for mapping onto a sphere.
    for (int i = 0; i < 9; i++)
    {
        shared_coefficients[face_index][i] = coefficients[i] * (4 * pi * weight_accumulation);
    }

    GroupMemoryBarrierWithGroupSync();

    // Combine coefficients.
    if (input.group_thread_id.z == 0)
    {
        // Add up shared coefficients.    
        float3 result[9];
        for (int j = 0; j < 9; j++)
        {
            for (int i = 0; i < 6; i++)
            {
                result[j] = result[i][j];
            }

            result[j] /= 6.0;
        }

        // Store calculated coefficients in output buffer.
        for (int i = 0; i < 9; i++)
        {
            output_buffer.Store(output_start_offset + (i * sizeof(float3)), result[i]);
        }
    }
}