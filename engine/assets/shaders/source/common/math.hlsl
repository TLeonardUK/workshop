// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _MATH_HLSL_
#define _MATH_HLSL_

static const float pi = 3.14159265359;
static const float half_pi = pi / 2.0;

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

#endif // _MATH_HLSL_