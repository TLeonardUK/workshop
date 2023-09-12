// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _COMPRESSION_HLSL_
#define _COMPRESSION_HLSL_

int float_to_byte(float value)
{
	value = (value + 1.0f) * 0.5f;
	return (int)(value * 255.0f);
}

float byte_to_float(int value)
{
	float ret = (value / 255.0f);
	ret = (ret * 2.0f) - 1.0f;
	return ret;
}

float compress_unit_vector(float3 input)
{
    int x = float_to_byte(input.x);
    int y = float_to_byte(input.y);
    int z = float_to_byte(input.z);
    int packed = (x << 16) | (y << 8) | z;
    return (float)packed;
}

float3 decompress_unit_vector(float input)
{
    float3 ret;

    // Unpack to 0-255 range.
    ret.x = floor(input / 65536.0f);
    ret.y = floor(fmod(input, 65536.0f) / 256.0f);
    ret.z = fmod(input, 256.0f);

    // Scale to -1 to 1 range.
    ret.x = (ret.x / 255.0f * 2.0f) - 1.0f;
    ret.y = (ret.y / 255.0f * 2.0f) - 1.0f;
    ret.z = (ret.z / 255.0f * 2.0f) - 1.0f;

    return normalize(ret);
}

#endif