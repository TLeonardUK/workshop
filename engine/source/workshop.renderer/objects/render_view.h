// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/rect.h"
#include "workshop.core/math/frustum.h"
#include "workshop.core/math/matrix4.h"

#include "workshop.renderer/render_object.h"

#include <memory>

namespace ws {

class renderer;
class ri_param_block;

// ================================================================================================
//  Represets a view into the scene to be renderer, including the associated projection
//  matrices, viewports and such.
// ================================================================================================
class render_view : public render_object
{
public:
    render_view(render_scene_manager* scene_manager, renderer& renderer);

    void set_viewport(const recti& viewport);
    recti get_viewport();

    void set_clip(float near, float far);
    void get_clip(float& near, float& far);

    void set_fov(float fov);
    float get_fov();

    void set_aspect_ratio(float ration);
    float get_aspect_ratio();

    matrix4 get_view_matrix();
    matrix4 get_perspective_matrix();

    ri_param_block* get_view_info_param_block();

    frustum get_frustum();

    // Overrides the normal set transform to update instance data when the transform changes.
    virtual void set_local_transform(const vector3& location, const quat& rotation, const vector3& scale) override;

    // The visibility index is used by render objects to refer to which views they are actively visible in
    // at the current point in time. Its assigned by the scene manager on creation and recycled ond destruction.
    inline static constexpr size_t k_always_visible_index = std::numeric_limits<size_t>::max();

    size_t visibility_index = k_always_visible_index;

private:
    void update_view_info_param_block();

private:
    renderer& m_renderer;

    recti m_viewport = recti::empty;

    float m_near_clip = 0.01f;
    float m_far_clip = 10000.0f;

    float m_field_of_view = 45.0f;
    float m_aspect_ratio = 1.33f;

    std::unique_ptr<ri_param_block> m_view_info_param_block;

};

}; // namespace ws
