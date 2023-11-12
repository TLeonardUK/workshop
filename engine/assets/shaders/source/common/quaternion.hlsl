// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _QUATERNION_HLSL_
#define _QUATERNION_HLSL_

float3 quat_rotate(float3 v, float4 q)
{
    float3 b = q.xyz;
    float b2 = dot(b, b);
    return (v * (q.w * q.w - b2) + b * (dot(v, b) * 2.f) + cross(b, v) * (q.w * 2.f));
}

float4 quat_conjugate(float4 q)
{
    return float4(-q.xyz, q.w);
}

#endif // _QUATERNION_HLSL_