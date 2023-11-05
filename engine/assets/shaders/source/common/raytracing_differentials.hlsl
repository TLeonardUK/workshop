// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#ifndef _RAYTRACING_DIFFERNTIALS_HLSL_
#define _RAYTRACING_DIFFERNTIALS_HLSL_

// Based on the RTXGI samples.

struct ray_diff
{
    float3 dOdx;
    float3 dOdy;
    float3 dDdx;
    float3 dDdy;
};

void compute_barycentric_differentials(ray_diff rd, float3 rayDir, float3 edge01, float3 edge02, float3 faceNormalW, out float2 dBarydx, out float2 dBarydy)
{
    // Igehy "Normal-Interpolated Triangles"
    float3 Nu = cross(edge02, faceNormalW);
    float3 Nv = cross(edge01, faceNormalW);

    // Plane equations for the triangle edges, scaled in order to make the dot with the opposite vertex equal to 1
    float3 Lu = Nu / (dot(Nu, edge01));
    float3 Lv = Nv / (dot(Nv, edge02));

    dBarydx.x = dot(Lu, rd.dOdx);     // du / dx
    dBarydx.y = dot(Lv, rd.dOdx);     // dv / dx
    dBarydy.x = dot(Lu, rd.dOdy);     // du / dy
    dBarydy.y = dot(Lv, rd.dOdy);     // dv / dy
}

void propagate_ray_diff(float3 D, float t, float3 N, inout ray_diff rd)
{
    // Part of Igehy Equation 10
    float3 dodx = rd.dOdx + t * rd.dDdx;
    float3 dody = rd.dOdy + t * rd.dDdy;

    // Igehy Equations 10 and 12
    float rcpDN = 1.f / dot(D, N);
    float dtdx = -dot(dodx, N) * rcpDN;
    float dtdy = -dot(dody, N) * rcpDN;
    dodx += D * dtdx;
    dody += D * dtdy;

    // Store differential origins
    rd.dOdx = dodx;
    rd.dOdy = dody;
}

void interpolate_tex_coord_differentials(float2 dBarydx, float2 dBarydy, Vertex vertices[3], out float2 dx, out float2 dy)
{
    float2 delta1 = vertices[1].uv0 - vertices[0].uv0;
    float2 delta2 = vertices[2].uv0 - vertices[0].uv0;
    dx = dBarydx.x * delta1 + dBarydx.y * delta2;
    dy = dBarydy.x * delta1 + dBarydy.y * delta2;
}

void prep_vertices_for_ray_diffs(Vertex vertices[3], out float3 edge01, out float3 edge02, out float3 faceNormal)
{
    // Apply instance transforms
    vertices[0].position = mul(ObjectToWorld3x4(), float4(vertices[0].position, 1.f)).xyz;
    vertices[1].position = mul(ObjectToWorld3x4(), float4(vertices[1].position, 1.f)).xyz;
    vertices[2].position = mul(ObjectToWorld3x4(), float4(vertices[2].position, 1.f)).xyz;

    // Find edges and face normal
    edge01 = vertices[1].position - vertices[0].position;
    edge02 = vertices[2].position - vertices[0].position;
    faceNormal = cross(edge01, edge02);
}

void compute_ray_direction_differentials(float3 nonNormalizedCameraRaydir, float3 right, float3 up, float2 viewportDims, out float3 dDdx, out float3 dDdy)
{
    float dd = dot(nonNormalizedCameraRaydir, nonNormalizedCameraRaydir);
    float divd = 2.f / (dd * sqrt(dd));
    float dr = dot(nonNormalizedCameraRaydir, right);
    float du = dot(nonNormalizedCameraRaydir, up);
    dDdx = ((dd * right) - (dr * nonNormalizedCameraRaydir)) * divd / viewportDims.x;
    dDdy = -((dd * up) - (du * nonNormalizedCameraRaydir)) * divd / viewportDims.y;
}

void compute_uv_differentials(Vertex vertices[3], float3 rayDirection, float hitT, out float2 dUVdx, out float2 dUVdy)
{
    // Initialize a ray differential
    ray_diff rd = (ray_diff)0;

    // Get ray direction differentials
    compute_ray_direction_differentials(rayDirection, GetCamera().right, GetCamera().up, GetCamera().resolution, rd.dDdx, rd.dDdy);

    // Get the triangle edges and face normal
    float3 edge01, edge02, faceNormal;
    prep_vertices_for_ray_diffs(vertices, edge01, edge02, faceNormal);

    // Propagate the ray differential to the current hit point
    propagate_ray_diff(rayDirection, hitT, faceNormal, rd);

    // Get the barycentric differentials
    float2 dBarydx, dBarydy;
    compute_barycentric_differentials(rd, rayDirection, edge01, edge02, faceNormal, dBarydx, dBarydy);

    // Interpolate the texture coordinate differentials
    interpolate_tex_coord_differentials(dBarydx, dBarydy, vertices, dUVdx, dUVdy);
}

#endif