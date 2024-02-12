// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/containers/command_queue.h"
#include "workshop.assets/asset_manager.h"

#include "workshop.core/math/rect.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/math/quat.h"
#include "workshop.core/math/obb.h"
#include "workshop.core/math/aabb.h"
#include "workshop.core/math/sphere.h"
#include "workshop.core/math/frustum.h"
#include "workshop.core/math/cylinder.h"
#include "workshop.core/math/hemisphere.h"
#include "workshop.core/drawing/color.h"

#include "workshop.render_interface/ri_texture.h"

namespace ws {

class renderer;
class model;
class material;
enum class visualization_mode;
enum class render_gpu_flags;
enum class render_draw_flags;
enum class render_view_flags;

// Used as an opaque reference to objects created through the use of the render command queue.
using render_object_id = size_t;

// Represents an object id that points to nothing.
static inline constexpr render_object_id null_render_object = 0;

// ================================================================================================
//  The render command queue is used by engine code to queue commands that modify
//  the state of the world being rendered.
// 
//  This provides a direct seperation between all rendering related code and all engine/game level
//  code. The render world state should only ever be modified via this interface. Handling
//  frame buffering/pipeline is not required when going through this interface.
// 
//  The API for this is designed to be simple and expose no render classes directly. Instead 
//  commands return opaque render_id types that can be used in future to reference objects
//  created via the commands.
// ================================================================================================
class render_command_queue : public command_queue
{
public:
    render_command_queue(renderer& render, size_t capacity);

public:

    // ===========================================================================================
    //  Global
    // ===========================================================================================

    // Regenerates all reflection light probes.
    void regenerate_reflection_probes();

    // Sets if we are in the editor or not, this adjusts how certain views are rendered.
    void set_editor_mode(bool in_editor);

    // ===========================================================================================
    //  Debug rendering.
    // ===========================================================================================

    // Draws various debug wireframe primitives.
    void draw_line(const vector3& start, const vector3& end, const color& color);
    void draw_aabb(const aabb& bounds, const color& color);
    void draw_obb(const obb& bounds, const color& color);
    void draw_sphere(const sphere& bounds, const color& color);
    void draw_frustum(const frustum& bounds, const color& color);
    void draw_triangle(const vector3& a, const vector3& b, const vector3& c, const color& color);
    void draw_cylinder(const cylinder& bounds, const color& color);
    void draw_capsule(const cylinder& bounds, const color& color);
    void draw_hemisphere(const hemisphere& bounds, const color& color, bool horizontal_bands = true);
    void draw_cone(const vector3& origin, const vector3& end, float radius, const color& color);
    void draw_arrow(const vector3& start, const vector3& end, const color& color);
    void draw_truncated_cone(const vector3& start, const vector3& end, float start_radius, float end_radius, const color& color);

    // ===========================================================================================
    //  Worlds
    // ===========================================================================================

    // Creates a world. Worlds are a high level filter filter of what objects are visible from what views.
    // If an object is not assigned to a specific world it exists in a default world that always 
    // exists and is used by default for rendering.
    render_object_id create_world(const char* name);

    // Destroys a world previously created with create_world.
    void destroy_world(render_object_id id);

    // ===========================================================================================
    //  Objects
    // ===========================================================================================

    // Sets the location and rotation of a render objects.
    void set_object_transform(render_object_id id, const vector3& location, const quat& rotation, const vector3& scale);

    // Sets the flags passed to the gpu to render the object.
    void set_object_gpu_flags(render_object_id id, render_gpu_flags flags);

    // Sets the flags that dictate what views an object is drawn to.
    void set_object_draw_flags(render_object_id id, render_draw_flags flags);

    // Sets the visibility of the render object.
    void set_object_visibility(render_object_id id, bool visibility);

    // Sets the world an object belongs to.
    void set_object_world(render_object_id id, render_object_id world_id);

    // ===========================================================================================
    //  Views
    // ===========================================================================================

    // Creates a new view of the scene that will be rendered to the backbuffer. 
    render_object_id create_view(const char* name);

    // Removes a view previously created with create_view.
    void destroy_view(render_object_id id);

    // Set the viewport in pixel space to which to render the view.    
    void set_view_viewport(render_object_id id, const recti& viewport);

    // Sets the camera to a perspective view with the given settings.
    void set_view_perspective(render_object_id id, float fov, float aspect_ratio, float min_depth, float max_depth);

    // Sets the camera to an orthographic view with the given settings.
    void set_view_orthographic(render_object_id id, rect ortho_rect, float min_depth, float max_depth);

    // Sets a pixmap that a views output will be copied to.
    void set_view_readback_pixmap(render_object_id id, pixmap* output);

    // Sets the render target the view renders to, if nullptr it will be rendered
    // to the back buffer.
    void set_view_render_target(render_object_id id, ri_texture_view render_target);

    // Sets the debug mode we should use for rendering the output.
    void set_view_visualization_mode(render_object_id id, visualization_mode mode);

    // Sets render flags defining what passes the view renders
    void set_view_flags(render_object_id id, render_view_flags mode);

    // Sets if the view is being actively rendered or not.
    void set_view_should_render(render_object_id id, bool active);

    // Forces a view to be rendered if it is marked as lazy_render.
    void force_view_render(render_object_id id);

    // ===========================================================================================
    //  Static meshes
    // ===========================================================================================

    // Creates a non-animated static mesh that will be rendered in the scene.
    render_object_id create_static_mesh(const char* name);

    // Destroys a static mesh previously created with create_static_mesh.
    void destroy_static_mesh(render_object_id id);

    // Set the model that will be rendered on the static mesh.
    void set_static_mesh_model(render_object_id id, const asset_ptr<model>& model);

    // Overrides the materials of a static mesh.
    void set_static_mesh_materials(render_object_id id, const std::vector<asset_ptr<material>>& materials);

    // ===========================================================================================
    //  Lights
    // ===========================================================================================

    // Sets how brightly a light shines.
    void set_light_intensity(render_object_id id, float value);

    // Sets how far away the light can effect.
    void set_light_range(render_object_id id, float value);

    // Sets how far away from the light is from the camera before its faded out.
    void set_light_importance_distance(render_object_id id, float value);

    // Sets the color a light produces.
    void set_light_color(render_object_id id, color color);

    // Sets if a light will cast shadows.
    void set_light_shadow_casting(render_object_id id, bool value);

    // Sets the size of of shadow map texture.
    void set_light_shadow_map_size(render_object_id id, size_t value);

    // Sets the maximum distance at which shadow will be casted by the light
    void set_light_shadow_max_distance(render_object_id id, float value);

    // ===========================================================================================
    //  Directional light
    // ===========================================================================================

    // Creates a directional light in the scene.
    render_object_id create_directional_light(const char* name);

    // Destroys a directional light previously created with create_directional_light
    void destroy_directional_light(render_object_id id);

    // Sets the number of cascades in the lights shadow map.
    void set_directional_light_shadow_cascades(render_object_id id, size_t value);

    // Sets the exponent from which the shadow map cascade split will be derived.
    // The lower the exponent the closer to linear the split becomes.
    void set_directional_light_shadow_cascade_exponent(render_object_id id, float value);

    // Sets the fraction of a cascade that is blended into the next cascade.
    void set_directional_light_shadow_cascade_blend(render_object_id id, float value);

    // ===========================================================================================
    //  Point light
    // ===========================================================================================

    // Creates a point light in the scene.
    render_object_id create_point_light(const char* name);

    // Destroys a point light previously created with create_point_light
    void destroy_point_light(render_object_id id);

    // ===========================================================================================
    //  Spot light
    // ===========================================================================================

    // Creates a spot light in the scene.
    render_object_id create_spot_light(const char* name);

    // Destroys a spot light previously created with create_point_light
    void destroy_spot_light(render_object_id id);

    // Sets the radius of the inner and outer bounds of the spotlight.
    // Radiuses are in radians are range from [0, pi]
    void set_spot_light_radius(render_object_id id, float inner_radius, float outer_radius);

    // ===========================================================================================
    //  Light Probe Grid
    // ===========================================================================================

    // Creates a new light probe grid in the scene.
    render_object_id create_light_probe_grid(const char* name);

    // Destroys a light probe grid previously created with create_light_probe_grid
    void destroy_light_probe_grid(render_object_id id);

    // Sets the density of a light probe grid, as a value that represents the seperation between each probe.
    void set_light_probe_grid_density(render_object_id id, float density);

    // ===========================================================================================
    //  Reflection Probe
    // ===========================================================================================

    // Creates a new reflection probe in the scene.
    render_object_id create_reflection_probe(const char* name);

    // Destroys a reflection probe previously created with create_reflection_probe
    void destroy_reflection_probe(render_object_id id);

private:
    renderer& m_renderer;

};

}; // namespace ws
