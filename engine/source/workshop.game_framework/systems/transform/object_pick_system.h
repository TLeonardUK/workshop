// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/vector3.h"
#include "workshop.core/math/quat.h"
#include "workshop.core/math/obb.h"
#include "workshop.core/math/ray.h"

#include "workshop.engine/ecs/object.h"
#include "workshop.engine/ecs/system.h"

#include "workshop.renderer/assets/model/model.h"

#include <unordered_set>
#include <vector>
#include <future>

namespace ws {

class transform_component;

// ================================================================================================
//  Handles picking objects for screen coordinates.
//  TODO: Most of this should be replaced by the physics system when its operational.
// ================================================================================================
class object_pick_system : public system
{
public:
    object_pick_system(object_manager& manager);

    virtual void step(const frame_time& time) override;

public:

    // Public Commands

    struct pick_result
    {
        object  hit_object = null_object;
        vector3 hit_location;
        vector3 hit_normal;
    };

    // Determines the closest object at the given screen space position.
    // 
    // This is slow and run in the background. Eventually we will replace this with
    // physics queries. Don't try to use this for any realtime behaviour.
    std::future<pick_result> pick(object camera, const vector2& screen_space_pos, const std::vector<object>& ignore_objects = {});

private:

    struct intersection_hit
    {
        bool coarse;
        object handle;
        float distance;
        vector3 hit_point;
    };
    
    struct intersection_test
    {
        bool coarse;
        object handle;
        matrix4 transform;
        asset_ptr<model> model;
        aabb bounds;
    };

    struct pick_request
    {
        ray target_ray;
        std::vector<object> ignore_objects;
        std::promise<pick_result> promise;
    };

    // Does an intersection test between a model at a given world space transform and a ray
    // This is sloooooooooooow.
    void model_ray_intersects(object handle, const ray& target_ray, model& mesh, const matrix4& transform, std::vector<intersection_hit>& hits);

    // Does fine triangle based intersection against the given objects.
    void fine_intersection_test(pick_request* request, std::vector<object>& objects);

private:

    std::mutex m_request_mutex;
    std::vector<std::unique_ptr<pick_request>> m_pending_requests;
    std::vector<std::unique_ptr<pick_request>> m_active_requests;

};

}; // namespace ws
