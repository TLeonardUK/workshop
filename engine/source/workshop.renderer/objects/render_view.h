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
#include "workshop.renderer/render_visibility_manager.h"
#include "workshop.render_interface/ri_texture.h"
#include "workshop.render_interface/ri_buffer.h"

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

    // Orthographic view.
    ortographic,

    // View expects a custom view and projection matrix to be provided.
    custom
};


// ================================================================================================
//  Configures what parts of the view pipeline are active for this view.
// ================================================================================================
enum class render_view_flags
{
    none                            = 0,

    // Standard path we use to render the scene.
    normal                          = 1 << 0,

    // Only renders a depth map of the scene.
    depth_only                      = 1 << 1,

    // Same as depth_only but depth is stored in a linear format.
    linear_depth_only               = 1 << 2,

    // Skips rendering of anything like debug primitives, hud, etc.
    scene_only                      = 1 << 3,

    // Applies a constant ambient term rather than calculating it from probes. This is useful when generating light probes.
    constant_ambient_lighting       = 1 << 4,
    
    // This view is for a probe/scene/etc capture, this forces all parts of the capture to be done this frame (eg. no staggered shadow updates).
    capture                         = 1 << 5,

    // Eye adapation is set to a constant value. This is useful when capturing renders where it hasn't stabalized.
    constant_eye_adaption           = 1 << 6,

    // Disables all imgui rendering inside this view. imgui should only be rendered in the top level view.
    no_imgui                        = 1 << 7,

    // Draws the bounds of the active cells in the rendering octtree.
    draw_cell_bounds                = 1 << 8,

    // Draws the bounds of individual objects in the rendering octree.
    draw_object_bounds              = 1 << 9,

    // Draws any debug rendering for objects that require it. This is used in the editor
    // to display things like lighting bounds.
    draw_object_debug               = 1 << 10,

    // If false direct lighting is used when rendering the scene.
    disable_direct_lighting         = 1 << 11,

    // If false ambient lighting is used when rendering the scene.
    disable_ambient_lighting        = 1 << 12,

    // If true the rendering is frozen on a given frame but allows the user to continue moving the camera about.
    freeze_rendering                = 1 << 13,
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
    render_view(render_object_id id, renderer& renderer);
    virtual ~render_view();

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

    // Gets/Sets the area of the world an orthographic view displays.
    void set_orthographic_rect(rect value);
    rect get_orthographic_rect();

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

    void set_visualization_mode(visualization_mode mode);
    visualization_mode get_visualization_mode();

    // GEts or sets if this view should be rendered this frame.
    void set_should_render(bool value);
    bool should_render();

    // Gets or sets if this view is active and visibility calculations/etc should be calculated.
    // This is independent of if it should render or not.
    void set_active(bool value);
    bool get_active();

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

    // Sets/Gets a pixmap that the rendered output of this view will be copied to.
    void set_readback_pixmap(pixmap* output);
    pixmap* get_readback_pixmap();
    ri_buffer* get_readback_buffer();

    // Returns true if the given object is visible within this view.
    bool is_object_visible(render_object* object);

    // Returns true if the view or something inside the view has changed.
    bool has_view_changed();

    // Gets the id of this view in the visibility system.
    render_visibility_manager::view_id get_visibility_view_id();

    virtual void bounds_modified() override;

private:
    void update_view_info_param_block();
    void update_render_target_flags();
    void update_visibility_flags();

private:
    recti m_viewport = recti::empty;
    rect m_ortho_rect = rect::empty;
    float m_near_clip = 0.01f;
    float m_far_clip = 10000.0f;
    float m_field_of_view = 45.0f;
    float m_aspect_ratio = 1.33f;

    matrix4 m_custom_view_matrix = matrix4::identity;
    matrix4 m_custom_projection_matrix = matrix4::identity;

    render_view_type m_view_type = render_view_type::perspective;
    render_view_flags m_flags = render_view_flags::normal;

    visualization_mode m_visualization_mode;

    render_visibility_manager::view_id m_visibility_view_id;

    ri_texture_view m_render_target;

    render_view_order m_render_view_order = render_view_order::normal;

    std::unique_ptr<ri_param_block> m_view_info_param_block;

    std::unique_ptr<render_resource_cache> m_resource_cache;

    bool m_should_render = true;
    bool m_active = true;

    pixmap* m_readback_pixmap = nullptr;
    std::unique_ptr<ri_texture> m_readback_rt;
    std::unique_ptr<ri_buffer> m_readback_buffer;

};

}; // namespace ws
