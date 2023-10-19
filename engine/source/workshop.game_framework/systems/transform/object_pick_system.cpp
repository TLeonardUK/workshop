// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.game_framework/systems/transform/object_pick_system.h"
#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.game_framework/systems/transform/bounds_system.h"
#include "workshop.game_framework/systems/camera/camera_system.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.engine/ecs/meta_component.h"
#include "workshop.engine/engine/world.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.renderer/assets/model/model.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/transform/bounds_component.h"
#include "workshop.game_framework/components/geometry/static_mesh_component.h"
#include "workshop.game_framework/components/geometry/billboard_component.h"
#include "workshop.core/async/task_scheduler.h"
#include "workshop.core/async/async.h"
#include "workshop.core/perf/profile.h"

namespace ws {

object_pick_system::object_pick_system(object_manager& manager)
    : system(manager, "pick system")
{
    add_predecessor<transform_system>();
    add_predecessor<bounds_system>();
}

void object_pick_system::model_ray_intersects(object handle, const ray& target_ray, model& instance, const matrix4& transform, std::vector<intersection_hit>& hits)
{
    profile_marker(profile_colors::system, "model_ray_intersection");

    geometry_vertex_stream* position_vertex_stream = instance.geometry->find_vertex_stream(geometry_vertex_stream_type::position);
    if (position_vertex_stream == nullptr)
    {
        return;
    }

    db_assert(position_vertex_stream->data_type == geometry_data_type::t_float3);
    vector3* position_array = reinterpret_cast<vector3*>(position_vertex_stream->data.data());

    // Check for any sub-mesh intersection.
    std::vector<model::mesh_info*> meshes_to_test;
    {
        profile_marker(profile_colors::system, "coarse bounds test");

        for (model::mesh_info& mesh : instance.meshes)
        {
            aabb world_bounds = obb(mesh.bounds, transform).get_aligned_bounds();
            if (target_ray.intersects(world_bounds))
            {
                meshes_to_test.push_back(&mesh);
            }
        }
    }

    if (meshes_to_test.empty())
    {
        return;
    }

    // Transform all verts outside of the loop to avoid doing the same calculations multiple times.
    // Note: We are intentionally not using an std::vector3 to avoid the default constructor penality which can get pretty insane
    //       with large number of vertices.
    size_t vertex_count = instance.geometry->get_vertex_count();
    std::unique_ptr<vector3[]> transformed_verts(new vector3[vertex_count]);

    {
        profile_marker(profile_colors::system, "build transformed vertices");
        parallel_for("build transformed vertices", task_queue::loading, vertex_count, [&transformed_verts, &transform, &position_array](size_t i) {
            transformed_verts[i] = position_array[i] * transform;
        });
    }

    // Test triangles of each mesh.
    std::mutex hits_mutex;
    parallel_for("mesh test", task_queue::loading, meshes_to_test.size(), [&meshes_to_test, &transform, &transformed_verts, &hits_mutex, &hits, &target_ray, &handle](size_t i) {
        profile_marker(profile_colors::system, "mesh test");

        model::mesh_info* mesh = meshes_to_test[i];

        aabb world_bounds = obb(mesh->bounds, transform).get_aligned_bounds();

        // *shudder*
        for (size_t i = 0; i < mesh->indices.size(); i += 3)
        {
            size_t i1 = mesh->indices[i];
            size_t i2 = mesh->indices[i + 1];
            size_t i3 = mesh->indices[i + 2];

            triangle tri(transformed_verts[i1], transformed_verts[i2], transformed_verts[i3]);

            vector3 hit_point;
            if (target_ray.intersects(tri, &hit_point))
            {
                std::scoped_lock lock(hits_mutex);

                intersection_hit& result = hits.emplace_back();
                result.coarse = false;
                result.handle = handle;
                result.distance = (hit_point - target_ray.start).length();
                result.hit_point = hit_point;
            }
        }
    });
}

void object_pick_system::fine_intersection_test(pick_request* request, std::vector<object>& objects)
{
    std::vector<intersection_test> tests;

    // Find meshes to do intersection tests against.
    for (size_t i = 0; i < objects.size(); i++)
    {
        object obj = objects[i];

        const transform_component* transform = m_manager.get_component<transform_component>(obj);
        static_mesh_component* mesh = m_manager.get_component<static_mesh_component>(obj);
        billboard_component* billboard = m_manager.get_component<billboard_component>(obj);
        bounds_component* bounds = m_manager.get_component<bounds_component>(obj);

        meta_component* meta = m_manager.get_component<meta_component>(obj);

        if (mesh != nullptr && mesh->model.is_loaded())
        {
            intersection_test& test = tests.emplace_back();
            test.coarse = false;
            test.handle = obj;
            test.transform = transform->local_to_world;
            test.model = mesh->model;
        }
        else if (billboard != nullptr && billboard->model.is_loaded())
        {
            intersection_test& test = tests.emplace_back();
            test.coarse = false;
            test.handle = obj;
            test.transform = billboard->transform * transform->local_to_world;
            test.model = billboard->model;
        }
        else if (bounds != nullptr)
        {
            intersection_test& test = tests.emplace_back();
            test.coarse = true;
            test.handle = obj;
            test.bounds = bounds->world_bounds.get_aligned_bounds();
        }
    }

    // Run the intersection tests async as they can be pretty beefy depending on what they are 
    // testing against.
    async("object pick intersection", task_queue::standard, [this, tests, request]() mutable {

        std::vector<intersection_hit> hits;

        // Find all hits.
        for (intersection_test& test : tests)
        {
            bool hit = false;
            vector3 hit_point = vector3::zero;

            // TODO: We can early-out of these tests by checking if the closest current hit distance is
            // closer than the closest point on the bounds.

            if (test.coarse)
            {
                if (request->target_ray.intersects(test.bounds, &hit_point))
                {
                    intersection_hit& result = hits.emplace_back();
                    result.coarse = true;
                    result.handle = test.handle;
                    result.distance = (hit_point - request->target_ray.start).length();
                    result.hit_point = hit_point;
                }
            }
            else
            {
                model_ray_intersects(test.handle, request->target_ray, *test.model.get(), test.transform, hits);
            }
        }

        // Sort by distance.
        std::sort(hits.begin(), hits.end(), [](intersection_hit& a, intersection_hit& b) {
            // Non-coarse hits get priority.
            if (a.coarse != b.coarse)
            {
                return a.coarse == false;
            }
            return a.distance < b.distance;
        });

        // Return the closest hit.
        if (hits.empty())
        {
            request->promise.set_value(null_object);
        }
        else
        {
            request->promise.set_value(hits[0].handle);
        }

        // Remove the request from the active queue.
        std::scoped_lock lock(m_request_mutex);
        auto iter = std::find_if(m_active_requests.begin(), m_active_requests.end(), [request](auto& a) {
            return a.get() == request;
        });
        if (iter != m_active_requests.end())
        {
            m_active_requests.erase(iter);
        }

    });
}

void object_pick_system::step(const frame_time& time)
{
    engine& engine = m_manager.get_world().get_engine();
    renderer& render = engine.get_renderer();
    bounds_system& bounds_sys = *m_manager.get_system<bounds_system>();

    std::scoped_lock lock(m_request_mutex);

    // Kick off all pending requests
    while (m_pending_requests.size() > 0)
    {
        std::unique_ptr<pick_request> request = std::move(m_pending_requests.back());
        m_pending_requests.pop_back();

        bool request_started = false;
        std::vector<object> intersecting = bounds_sys.intersects(request->target_ray);

        // Do fine intersection testing of each objects model async.
        if (!intersecting.empty())
        {
            fine_intersection_test(request.get(), intersecting);
            request_started = true;
        }

        if (request_started)
        {
            m_active_requests.push_back(std::move(request));
        }
        else
        {
            // If failed to kick off request just return a null object.
            request->promise.set_value(null_object);
        }
    }
}

std::future<object> object_pick_system::pick(object camera, const vector2& screen_space_pos)
{
    camera_system& cam_system = *m_manager.get_system<camera_system>();

    std::unique_ptr<pick_request> request = std::make_unique<pick_request>();
    request->target_ray = cam_system.screen_to_ray(camera, screen_space_pos);

    std::future<object> future = request->promise.get_future();

    std::scoped_lock lock(m_request_mutex);
    m_pending_requests.push_back(std::move(request));

    return future;
}

}; // namespace ws
