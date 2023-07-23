// ================================================================================================
//  workshop
//  Copyright (C) 2022 Tim Leonard
// ================================================================================================
#include "data:shaders/source/common/math.hlsl"
#include "data:shaders/source/common/lighting.hlsl"

struct cull_lights_input
{
	uint3 group_thread_id : SV_GroupThreadID;
	uint3 group_id : SV_GroupID;
	uint3 dispatch_thread_id : SV_DispatchThreadID;
	uint group_index : SV_GroupIndex;
};

struct shared_light_state
{
    float range;
    float3 position;
    int type;
};

groupshared shared_light_state g_group_shared_lights[GROUP_SIZE_X * GROUP_SIZE_Y * GROUP_SIZE_Z];

light_state get_light_state(int index)
{
    instance_offset_info info = light_buffer.Load<instance_offset_info>(index * sizeof(instance_offset_info));
    return table_byte_buffers[NonUniformResourceIndex(info.data_buffer_index)].Load<light_state>(info.data_buffer_offset);
}

float point_to_cluster_squared_distance(float3 location, uint cluster_index)
{
    float squared_distance = 0.0;

    light_cluster cluster = light_cluster_buffer.Load<light_cluster>(cluster_index * sizeof(light_cluster));

    for (int i = 0; i < 3; ++i)
    {
        float v = location[i];

        if (v < cluster.aabb_min[i])
        {
            squared_distance += (cluster.aabb_min[i] - v) * (cluster.aabb_min[i] - v);
        }

        if (v > cluster.aabb_max[i])
        {
            squared_distance += (v - cluster.aabb_max[i]) * (v - cluster.aabb_max[i]);
        }
    }

    return squared_distance;
}

bool intersect_light_cluster(uint light, uint cluster)
{
    // Directional lights are always applied.
    if (g_group_shared_lights[light].type == light_type::directional)
    {
        return true;
    }

    float  radius = g_group_shared_lights[light].range;
    float3 pos = g_group_shared_lights[light].position;
    float3 center = mul(view_matrix, float4(pos, 1.0f)).xyz;
    float  squared_distance = point_to_cluster_squared_distance(center, cluster);
    float radius_squared = (radius * radius);

    return squared_distance <= radius_squared;
}

[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, GROUP_SIZE_Z)]
void cull_lights_cshader(cull_lights_input input)
{
    uint thread_count = GROUP_SIZE_X * GROUP_SIZE_Y * GROUP_SIZE_Z;
    uint batch_count = (light_count + thread_count - 1) / thread_count;

    uint cluster_index = input.group_index + GROUP_SIZE_X * GROUP_SIZE_Y * GROUP_SIZE_Z * input.group_id.z;

    uint visible_light_count = 0;
    uint visible_light_indices[MAX_LIGHTS_PER_CLUSTER];

    // This loop is fairly simple, we batch the light list up into groups equal to the number
    // of threads we are executing.
    // Each thread loads in one light in the batch into shared memory. Then all threads iterate
    // through the shared light list and add the light to their cluster if it intersects their cluster.
    for (uint batch = 0; batch < batch_count; batch++)
    {
        uint light_index = batch * thread_count + input.group_index;

        // Clamp to maximum to avoid reading out of bounds.
        light_index = min(light_index, light_count - 1);

        // Populate the shared light index.
        light_state state = get_light_state(light_index);
        shared_light_state shared_state;
        shared_state.position = state.position;
        shared_state.range = state.range;
        shared_state.type = state.type;
        g_group_shared_lights[input.group_index] = shared_state;

        // Wait for all threads to fill up the shared light batch.
        GroupMemoryBarrierWithGroupSync();
    
        // Every thread test for overlap with their cluster against the list of shared lights.
        uint valid_shared_light_count = min(thread_count, light_count - (batch * thread_count));
        for (uint shared_light_index = 0; shared_light_index < valid_shared_light_count; shared_light_index++)
        {
            if (intersect_light_cluster(shared_light_index, cluster_index))
            {
                if (visible_light_count < MAX_LIGHTS_PER_CLUSTER)
                {
                    visible_light_indices[visible_light_count] = batch * thread_count + shared_light_index;
                    visible_light_count++;
                }
            }
        }
    }

    // Wait for all thread to have finished calculating intersections.
    GroupMemoryBarrierWithGroupSync();

    // Each thread now adds its data to the culled light list.
    uint offset = 0;
    light_cluster_visibility_count_buffer.InterlockedAdd(0, visible_light_count, offset);

    for (uint i = 0; i < visible_light_count; i++)
    {
        uint light_index = visible_light_indices[i];
        light_cluster_visibility_buffer.Store((offset + i) * sizeof(uint), light_index);
    }
 
    // Update cluster with the lights visible in it.
    light_cluster cluster = light_cluster_buffer.Load<light_cluster>(cluster_index * sizeof(light_cluster));
    cluster.visible_light_offset = offset;
    cluster.visible_light_count = visible_light_count;
    light_cluster_buffer.Store(cluster_index * sizeof(light_cluster), cluster);
}  