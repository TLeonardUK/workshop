// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _MATH_HLSL_
#define _MATH_HLSL_

static const float pi = 3.14159265359;
static const float half_pi = pi / 2.0;
static const float pi2 = pi * 2.0;

static const float2 poisson_disk[16] = 
{
   float2( -0.94201624, -0.39906216 ), 
   float2( 0.94558609, -0.76890725 ), 
   float2( -0.094184101, -0.92938870 ), 
   float2( 0.34495938, 0.29387760 ), 
   float2( -0.91588581, 0.45771432 ), 
   float2( -0.81544232, -0.87912464 ), 
   float2( -0.38277543, 0.27676845 ), 
   float2( 0.97484398, 0.75648379 ), 
   float2( 0.44323325, -0.97511554 ), 
   float2( 0.53742981, -0.47373420 ), 
   float2( -0.26496911, -0.41893023 ), 
   float2( 0.79197514, 0.19090188 ), 
   float2( -0.24188840, 0.99706507 ), 
   float2( -0.81409955, 0.91437590 ), 
   float2( 0.19984126, 0.78641367 ), 
   float2( 0.14383161, -0.14100790 ) 
};

float square(float value)
{
    return value * value;
}

float random(float4 seed)
{   
    // *shudder*
	float dot_product = dot(seed, float4(12.9898, 78.233, 45.164, 94.673));
    return frac(sin(dot_product) * 43758.5453);
}

float3 make_perpendicular(float3 normal)
{
    float3 c1 = cross(normal, float3(0.0, 0.0, 1.0));
    float3 c2 = cross(normal, float3(0.0, 1.0, 0.0));

    if (length(c1) > length(c2))
    {
        return c1;
    }
    else
    {
        return c2;
    }
}

// Takes a location in screen space and converts it into view space.
float4 screen_space_to_view_space(float4 location, float2 screen_dimensions, float4x4 inverse_projection)
{
    // Convert to NDC
    float2 uv = location.xy / screen_dimensions.xy;

    // Convert to clipspace.
    float4 clip = float4(
        uv.x * 2 - 1,
        uv.y * 2 - 1,
        location.z,
        location.w
    );

    // Convert to view
    float4 view = mul(inverse_projection, clip);
    
    // Perspective projection.
    view = view / view.w;

    return view;
}

float4 ndc_to_view_space(float2 uv, float4x4 inverse_projection)
{
    // Convert to clipspace.
    float4 clip = float4(
        uv.x * 2 - 1,
        uv.y * 2 - 1,
        0.0f,
        1.0f
    );

    // Convert to view
    float4 view = mul(inverse_projection, clip);
    
    // Perspective projection.
    view = view / view.w;

    return view;
}

float4 ndc_to_world_space(float2 uv, float4x4 inverse_projection, float4x4 inverse_view)
{
    // Convert to clipspace.
    float4 clip = float4(
        uv.x * 2 - 1,
        uv.y * 2 - 1,
        0.0f,
        1.0f
    );

    // Convert to view
    float4 view = mul(inverse_projection, clip);
    
    // Perspective projection.
    view = view / view.w;

    // Convert to world
    float4 world = mul(inverse_view, view);

    return world;
}

// Create a line segment from the eye to the screen location, then finds it 
// intersection with a z plane located the given distance from the origin.
float3 intersect_line_to_z_plane(float3 a, float3 b, float distance)
{
    float3 normal = float3(0.0, 0.0, 1.0);
    float3 a_to_b = b - a;
    float t = (distance - dot(normal, a)) / dot(normal, a_to_b);
    return a + (t * a_to_b);
}

// Converts clip space into viewport space.
float4 clip_space_to_viewport_space(float4 input, float2 screen_dimensions)
{
    // clip space to NDC        
    input.xyz = input.xyz / input.w;
    input.w = 1.0f;
    // NDC to viewport space.
    input.xy = (input.xy + 1.0f) * 0.5f;
    input.xy = input.xy * screen_dimensions;

    return input;
}

// Converts a cubemap face and a uv to the normal you need to sample the correct
// texel in the cubemap.
float3 get_cubemap_normal(int face_index, int size, int2 xy)
{
    float pixel_size = 1.0f / size;

    float3 dir = 0.0f;

    switch (face_index)
    {
    // Positive X
    case 0: 
        dir.z = 1.0f - (2.0f * float(xy.x) + 1.0f) * pixel_size;
        dir.y = 1.0f - (2.0f * float(xy.y) + 1.0f) * pixel_size;
        dir.x = 1.0f;
        break;
    // Negative X
    case 1: 
        dir.z = -1.0f + (2.0f * float(xy.x) + 1.0f) * pixel_size;
        dir.y = 1.0f - (2.0f * float(xy.y) + 1.0f) * pixel_size;
        dir.x = -1;
        break;
    // Positive Y
    case 2: 
        dir.z = -1.0f + (2.0f * float(xy.y) + 1.0f) * pixel_size;
        dir.y = 1.0f;
        dir.x = -1.0f + (2.0f * float(xy.x) + 1.0f) * pixel_size;
        break;
    // Negative Y
    case 3: 
        dir.z = 1.0f - (2.0f * float(xy.y) + 1.0f) * pixel_size;
        dir.y = -1.0f;
        dir.x = -1.0f + (2.0f * float(xy.x) + 1.0f) * pixel_size;
        break;
    // Positive Z
    case 4: 
        dir.z = 1.0f;
        dir.y = 1.0f - (2.0f * float(xy.y) + 1.0f) * pixel_size;
        dir.x = -1.0f + (2.0f * float(xy.x) + 1.0f) * pixel_size;
        break;
    // Negative Z
    case 5: 
        dir.z = -1.0f;
        dir.y = 1.0f - (2.0f * float(xy.y) + 1.0f) * pixel_size;
        dir.x = 1.0f - (2.0f * float(xy.x) + 1.0f) * pixel_size;
        break;
    }
    
    return normalize(dir);
}

float3 get_cubemap_normal_from_uv(int face_index, float2 uv)
{
    float3 dir = 0.0f;

    // [0,1] -> [-1,1]
    uv = (uv * 2) - 1.0f;

    switch (face_index)
    {
    // Positive X
    case 0: 
        dir.z = 1.0f - uv.x;
        dir.y = 1.0f - uv.y;
        dir.x = 1.0f;
        break;
    // Negative X
    case 1: 
        dir.z = -1.0f + uv.x;
        dir.y = 1.0f - uv.y;
        dir.x = -1;
        break;
    // Positive Y
    case 2: 
        dir.z = -1.0f + uv.y;
        dir.y = 1.0f;
        dir.x = -1.0f + uv.x;
        break;
    // Negative Y
    case 3: 
        dir.z = 1.0f - uv.y;
        dir.y = -1.0f;
        dir.x = -1.0f + uv.x;
        break;
    // Positive Z
    case 4: 
        dir.z = 1.0f;
        dir.y = 1.0f - uv.y;
        dir.x = -1.0f + uv.x;
        break;
    // Negative Z
    case 5: 
        dir.z = -1.0f;
        dir.y = 1.0f - uv.y;
        dir.x = 1.0f - uv.x;
        break;
    }
    
    return normalize(dir);
}

float calculate_luminance(float3 color)
{
    return dot(color, float3(0.2127f, 0.7152f, 0.0722f));
}

float3 convert_rgb_to_xyz(float3 _rgb)
{
	// http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
	float3 xyz;
	xyz.x = dot(float3(0.4124564, 0.3575761, 0.1804375), _rgb);
	xyz.y = dot(float3(0.2126729, 0.7151522, 0.0721750), _rgb);
	xyz.z = dot(float3(0.0193339, 0.1191920, 0.9503041), _rgb);
	return xyz;
}

float3 convert_xyz_to_rgb(float3 _xyz)
{
	float3 rgb;
	rgb.x = dot(float3( 3.2404542, -1.5371385, -0.4985314), _xyz);
	rgb.y = dot(float3(-0.9692660,  1.8760108,  0.0415560), _xyz);
	rgb.z = dot(float3( 0.0556434, -0.2040259,  1.0572252), _xyz);
	return rgb;
}

float3 convert_xyz_to_yxy(float3 _xyz)
{
	// http://www.brucelindbloom.com/index.html?Eqn_XYZ_to_xyY.html
	float inv = 1.0 / dot(_xyz, float3(1.0, 1.0, 1.0));
	return float3(_xyz.y, _xyz.x * inv, _xyz.y * inv);
}

float3 convert_yxy_to_xyz(float3 _Yxy)
{
	// http://www.brucelindbloom.com/index.html?Eqn_xyY_to_XYZ.html
	float3 xyz;
	xyz.x = _Yxy.x * _Yxy.y / _Yxy.z;
	xyz.y = _Yxy.x;
	xyz.z = _Yxy.x * (1.0 - _Yxy.y - _Yxy.z) / _Yxy.z;
	return xyz;
}

float3 convert_rgb_to_yxy(float3 _rgb)
{
	return convert_xyz_to_yxy(convert_rgb_to_xyz(_rgb));
}

float3 convert_yxy_to_rgb(float3 _Yxy)
{
	return convert_xyz_to_rgb(convert_yxy_to_xyz(_Yxy));
}

float4x4 extract_rotation_matrix(float4x4 m)
{
    float sx = length(float3(m[0][0], m[0][1], m[0][2]));
    float sy = length(float3(m[1][0], m[1][1], m[1][2]));
    float sz = length(float3(m[2][0], m[2][1], m[2][2]));

    // if determine is negative, we need to invert one scale
    float det = determinant(m);
    if (det < 0) {
        sx = -sx;
    }

    float invSX = 1.0 / sx;
    float invSY = 1.0 / sy;
    float invSZ = 1.0 / sz;

    m[0][0] *= invSX;
    m[0][1] *= invSX;
    m[0][2] *= invSX;
    m[0][3] = 0;

    m[1][0] *= invSY;
    m[1][1] *= invSY;
    m[1][2] *= invSY;
    m[1][3] = 0;

    m[2][0] *= invSZ;
    m[2][1] *= invSZ;
    m[2][2] *= invSZ;
    m[2][3] = 0;

    m[3][0] = 0;
    m[3][1] = 0;
    m[3][2] = 0;
    m[3][3] = 1;

    return m;
}

float inverse_lerp(float a, float b, float value)
{
        return (value - a) / (b - a);
}

float barycentric_lerp(in float v0, in float v1, in float v2, in float3 barycentrics)
{
    return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

float2 barycentric_lerp(in float2 v0, in float2 v1, in float2 v2, in float3 barycentrics)
{
    return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

float3 barycentric_lerp(in float3 v0, in float3 v1, in float3 v2, in float3 barycentrics)
{
    return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

float4 barycentric_lerp(in float4 v0, in float4 v1, in float4 v2, in float3 barycentrics)
{
    return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

float max3(float3 input)
{
    return max(max(input.x, input.y), input.z);
}

float maxabs3(float3 input)
{
    return max(max(abs(input.x), abs(input.y)), abs(input.z));
}

// Computes a spherically distributed set of directions ont he unit sphere.
// Based on the ray selection code in RTXGI
float3 spherical_fibonacci(float sample_index, float num_samples)
{
    const float b = (sqrt(5.f) * 0.5f + 0.5f) - 1.f;
    float phi = pi2 * frac(sample_index * b);
    float cos_theta = 1.f - (2.f * sample_index + 1.f) * (1.f / num_samples);
    float sin_theta = sqrt(saturate(1.f - (cos_theta * cos_theta)));

    return normalize(float3((cos(phi) * sin_theta), (sin(phi) * sin_theta), cos_theta));
}
 
// Returns either -1 or 1 based on the sign of the input value.
// If the input is zero, 1 is returned.
float sign_not_zero(float v)
{
    return (v >= 0.f) ? 1.f : -1.f;
}

float2 sign_not_zero(float2 v)
{
    return float2(sign_not_zero(v.x), sign_not_zero(v.y));
}

#endif // _MATH_HLSL_