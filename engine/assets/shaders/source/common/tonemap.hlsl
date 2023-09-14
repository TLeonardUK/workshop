// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _TONEMAP_HLSL_
#define _TONEMAP_HLSL_

static const float3 tonemap_gamma = 2.2;

float tonemap_reinhard2(float input, float whitepoint)
{
	return (input * (1.0 + input / whitepoint)) / (1.0 + input);
}

float3 tonemap_reinhard2(float3 input, float whitepoint)
{
	return (input * (1.0 + input / whitepoint)) / (1.0 + input);
}

float3 tonemap_reinhard(float3 input)
{
    float exposure = 1.5;
    input *= exposure / (1.0 + input / exposure);
    input = pow(input, 1.0 / tonemap_gamma);
    return input;
}

float3 tonemap_uncharted2_operator(float3 input)
{
    float a = 0.15;
    float b = 0.50;
    float c = 0.10;
    float d = 0.20;
    float e = 0.02;
    float f = 0.30;

    return ((input * (a * input + c * b) + d * e) / (input * (a * input + b) + d * f)) - e / f;
}

float3 tonemap_uncharted2(float3 input)
{
    float w = 11.2;

    float exposure_bias = 2.0f;
    float3 current = tonemap_uncharted2_operator(exposure_bias * input);

    float3 white_scale = 1.0 / tonemap_uncharted2_operator(w);
    float3 scaled = current * white_scale;

    return pow(abs(scaled), tonemap_gamma);
}

float3 tonemap_filmic(float3 input)
{
    input = max(0, input - 0.004f);
    input = (input * (6.2f * input + 0.5f)) / (input * (6.2f * input + 1.7f) + 0.06f);

    return input;//pow(input, tonemap_gamma);
}

float3 linear_to_srgb(float3 x)
{
    return x < 0.0031308 ? 12.92 * x : 1.055 * pow(x, 1.0 / 2.4) - 0.055;
}

float3 srgb_to_linear(float3 x)
{
    return x < 0.04045 ? x / 12.92 : pow( (x + 0.055) / 1.055, 2.4 );
}

#endif