// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/rect.h"
#include "workshop.core/math/frustum.h"
#include "workshop.core/math/matrix4.h"
#include "workshop.core/utils/traits.h"

#include "workshop.renderer/render_object.h"
#include "workshop.render_interface/ri_texture.h"

#include <memory>

namespace ws {

class renderer;
class ri_param_block;
class render_resource_cache;

// ================================================================================================
//  Represets how the view and projection matrices are generated for the view.
// ================================================================================================
enum class render_view_type
{
    // Perspective information is generated from the parameters set on the view.
    perspective,

    // View expects a custom view and projection matrix to be provided.
    custom
};


// ================================================================================================
//  Configures what parts of the view pipeline are active for this view.
// ================================================================================================
enum class render_view_flags
{
    none                    = 0,

    // Standard path we use to render the scene.
    normal                  = 1,

    // Only renders a depth map of the scene.
    depth_only              = 2,

    // Same as depth_only but depth is stored in a linear format.
    linear_depth_only       = 4,

    // Skips rendering of anything like debug primitives, hud, etc.
    scene_only              = 8,

    // Does not apply any ambient lighting. This is useful when generating light probes.
    no_ambient_lighting     = 16,
};
DEFINE_ENUM_FLAGS(render_view_flags);

// ================================================================================================
//  Order of rendering a view. This is treated as an int, the named values are just rough
//  guidelines.
// ================================================================================================
enum class render_view_order
{
    shadows = -2000,

    light_probe = -1000,

    normal = 0,
};

// ================================================================================================
//  Represets a view into the scene to be renderer, including the associated projection
//  matrices, viewports and such.
// ================================================================================================
class render_view : public render_object
{
public:
    render_view(render_object_id id, render_scene_manager* scene_manager, renderer& renderer);

    // Sets the mode used to generate our perspective and view matrices.
    void set_view_type(render_view_type type);
    render_view_type get_view_type();

    // Gets/Sets a perspective matrix. Set matrices only take effect if
    // the view type is set to custom.
    void set_projection_matrix(matrix4 type);
    matrix4 get_projection_matrix();

    // Gets/Sets a view matrix. Set matrices only take effect if
    // the view type is set to custom.
    void set_view_matrix(matrix4 type);
    matrix4 get_view_matrix();

    // Sets the render target this view will render to. If not set
    // the view will be rendered to the swapchain.
    void set_render_target(ri_texture_view view);
    ri_texture_view get_render_target();
    bool has_render_target();

    // Gets or sets the order of rendering of this view.
    void set_view_order(render_view_order order);
    render_view_order get_view_order();

    // Gets or sets the flags determining how this view is rendered.
    void set_flags(render_view_flags flags);
    render_view_flags get_flags();
    bool has_flag(render_view_flags flags);

    void set_viewport(const recti& viewport);
    recti get_viewport();

    // GEts or sets if this view should be rendered this frame.
    void set_should_render(bool value);
    bool should_render();

    // Gets and sets all the perspective matrice parameters.
    void set_clip(float near, float far);
    void get_clip(float& near, float& far);
    void set_fov(float fov);
    float get_fov();
    void set_aspect_ratio(float ration);
    float get_aspect_ratio();

    ri_param_block* get_view_info_param_block();

    frustum get_frustum();
    frustum get_view_frustum();

    render_resource_cache& get_resource_cache();

    // Overrides the normal set transform to update instance data when the transform changes.
    virtual void set_local_transform(const vector3& location, const quat& rotation, const vector3& scale) override;

    // The visibility index is used by render objects to refer to which views they are actively visible in
    // at the current point in time. Its assigned by the scene manager on creation and recycled ond destruction.
    inline static constexpr size_t k_always_visible_index = std::numeric_limits<size_t>::max();

    size_t visibility_index = k_always_visible_index;

    // Returns true if the given object is visible within this view.
    bool is_object_visible(render_object* object);

    // Gets an opaque numeric value determining when this view changed last.
    size_t get_last_change();

    // Marks this view as dirty (eg. something inside it has changed) and uses the
    // passed numeric value as its new last_change value.
    void mark_dirty(size_t last_change);

    // Returns true if view is currently dirty.
    bool is_dirty();

    // Clears dirty flag of the view.
    void clear_dirty();

    virtual void bounds_modified() override;

private:
    void update_view_info_param_block();

private:
    renderer& m_renderer;

    recti m_viewport = recti::empty;

    float m_near_clip = 0.01f;
    float m_far_clip = 10000.0f;

    float m_field_of_view = 45.0f;
    float m_aspect_ratio = 1.33f;

    ri_texture_view m_render_target;

    render_view_type m_view_type = render_view_type::perspective;
    render_view_flags m_flags = render_view_flags::normal;

    matrix4 m_custom_view_matrix = matrix4::identity;
    matrix4 m_custom_projection_matrix = matrix4::identity;

    render_view_order m_render_view_order = render_view_order::normal;

    std::unique_ptr<ri_param_block> m_view_info_param_block;

    std::unique_ptr<render_resource_cache> m_resource_cache;

    bool m_dirty = true;
    size_t m_last_change = 0;

    bool m_should_render = true;

};

}; // namespace ws
