// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================

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