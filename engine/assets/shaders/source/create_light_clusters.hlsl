// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/common/math.hlsl"

struct cull_lights_input
{
	uint3 group_thread_id : SV_GroupThreadID;
	uint3 group_id : SV_GroupID;
	uint3 dispatch_thread_id : SV_DispatchThreadID;
	uint group_index : SV_GroupIndex;
};

[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, GROUP_SIZE_Z)]
void create_clusters_cshader(cull_lights_input input)
{
    float2 tile_size = ceil(view_dimensions.x / light_grid_size.x);
    uint cluster_index = input.group_id.x +
                         input.group_id.y * DISPATCH_SIZE_X + 
                         input.group_id.z * (DISPATCH_SIZE_X * DISPATCH_SIZE_Y);

    // Calculate the min and max locations in screen space.
    float4 max_screen_space = float4(float2(input.group_id.x + 1, input.group_id.y + 1) * tile_size, 0.0, 1.0); 
    float4 min_screen_space = float4(input.group_id.xy * tile_size, 0.0, 1.0); 

    // Convert to view space.
    float3 max_view_space = screen_space_to_view_space(max_screen_space, view_dimensions, inverse_projection_matrix).xyz;
    float3 min_view_space = screen_space_to_view_space(min_screen_space, view_dimensions, inverse_projection_matrix).xyz;

    // Calculate near and far planes in view space.
    float total_slices = DISPATCH_SIZE_Z;
    float tile_near = view_z_near * pow(view_z_far / view_z_near, input.group_id.z / total_slices);
    float tile_far  = view_z_near * pow(view_z_far / view_z_near, (input.group_id.z + 1) / total_slices);

    // Find intersections with z planes.
    float3 min_near = intersect_line_to_z_plane(0.0, min_view_space, tile_near);
    float3 min_far  = intersect_line_to_z_plane(0.0, min_view_space, tile_far);
    float3 max_near = intersect_line_to_z_plane(0.0, max_view_space, tile_near);
    float3 max_far  = intersect_line_to_z_plane(0.0, max_view_space, tile_far);

    // Store result
    light_cluster cluster;
    cluster.z_range = float2(tile_near, tile_far);
    cluster.cell = input.group_id;
    cluster.aabb_min = min(min(min_near, min_far), min(max_near, max_far));
    cluster.aabb_max = max(max(min_near, min_far), max(max_near, max_far));
    cluster.visible_light_offset = 0;
    cluster.visible_light_count = 0;
    light_cluster_buffer.Store(cluster_index * sizeof(light_cluster), cluster);
    
    // Clear the visibility count buffer for the next culling step.
    light_cluster_visibility_count_buffer.Store(0, 0);
}  
