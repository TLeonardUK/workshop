// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"
#include "workshop.core/containers/oct_tree.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.renderer/render_effect.h"
#include "workshop.renderer/render_object.h"
#include "workshop.renderer/render_command_queue.h"

#include <unordered_map>

namespace ws {

class renderer;
class asset_manager;
class shader;
class render_view;
class render_static_mesh;
class render_directional_light;
class render_point_light;
class render_spot_light;
class render_light_probe_grid;
class render_reflection_probe;
class render_world;

// ================================================================================================
//  Handles management of the render scene and the objects within it. 
//  This class should generally not be accessed directly, but via the render_command_queue.
// ================================================================================================
class render_scene_manager
{
public:

public:

    render_scene_manager(renderer& render);

    // Registers all the steps required to initialize the system.
    void register_init(init_list& list);

    // Gets a pointer to a render object from its id, returns nullptr on failure.
    render_object* resolve_id(render_object_id id);

    // Gets a pointer to a render object from its id, returns nullptr on failure.
    template<typename T>
    T* resolve_id_typed(render_object_id id)
    {
        return dynamic_cast<T*>(resolve_id(id));
    }

    // Gets a list of all objects. 
    // This is slow, don't use it for anything but debugging.
    std::vector<render_object*> get_objects();

public:

    // ===========================================================================================
    //  Worlds
    // ===========================================================================================

    // Creates a world. Worlds are a high level filter filter of what objects are visible from what views.
    // If an object is not assigned to a specific world it exists in a default world that always 
    // exists and is used by default for rendering.
    void create_world(render_object_id id, const char* name);

    // Destroys a world previously created with create_world.
    void destroy_world(render_object_id id);

    // Gets a list of all active worlds.
    std::vector<render_world*> get_worlds();

    // ===========================================================================================
    //  Objects
    // ===========================================================================================

    // Sets the local-space transform of an object within the render scene.
    void set_object_transform(render_object_id id, const vector3& location, const quat& rotation, const vector3& scale);

    // Sets the flags on an object that defines how it is rendered.
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

    // Creates a new view that has the given id. id's are expected to be unique.
    void create_view(render_object_id id, const char* name);

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

    // Sets what debug visualization the view is rendered with.
    void set_view_visualization_mode(render_object_id id, visualization_mode mode);

    // Sets render flags defining what passes the view renders
    void set_view_flags(render_object_id id, render_view_flags mode);

    // Gets a list of all active views.
    std::vector<render_view*> get_views();

    // ===========================================================================================
    //  Static meshes
    // ===========================================================================================

    // Creates a new static mesh that has the given id. id's are expected to be unique.
    void create_static_mesh(render_object_id id, const char* name);

    // Removes a static mesh previously created with create_static_mesh.
    void destroy_static_mesh(render_object_id id);

    // Sets the model a static mesh is rendering.
    void set_static_mesh_model(render_object_id id, const asset_ptr<model>& model);

    // Overrides the materials a static mesh renders with.
    void set_static_mesh_materials(render_object_id id, const std::vector<asset_ptr<material>>& materials);

    // Gets a list of all active static meshes.
    std::vector<render_static_mesh*> get_static_meshes();

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
    void create_directional_light(render_object_id id, const char* name);

    // Destroys a directional light previously created with create_directional_light
    void destroy_directional_light(render_object_id id);

    // Sets the number of cascades in the lights shadow map.
    void set_directional_light_shadow_cascades(render_object_id id, size_t value);

    // Sets the exponent from which the shadow map cascade split will be derived.
    // The lower the exponent the closer to linear the split becomes.
    void set_directional_light_shadow_cascade_exponent(render_object_id id, float value);

    // Sets the fraction of a cascade that is blended into the next cascade.
    void set_directional_light_shadow_cascade_blend(render_object_id id, float value);

    // Gets a list of all active directional lights.
    std::vector<render_directional_light*> get_directional_lights();

    // ===========================================================================================
    //  Point light
    // ===========================================================================================

    // Creates a point light in the scene.
    void create_point_light(render_object_id id, const char* name);

    // Destroys a point light previously created with create_point_light
    void destroy_point_light(render_object_id id);

    // Gets a list of all active point lights.
    std::vector<render_point_light*> get_point_lights();

    // ===========================================================================================
    //  Spot light
    // ===========================================================================================

    // Creates a spot light in the scene.
    void create_spot_light(render_object_id id, const char* name);

    // Destroys a point light previously created with create_point_light
    void destroy_spot_light(render_object_id id);

    // Sets the radius of the inner and outer bounds of the spotlight.
    void set_spot_light_radius(render_object_id id, float inner_radius, float outer_radius);

    // Gets a list of all active spot lights.
    std::vector<render_spot_light*> get_spot_lights();

    // ===========================================================================================
    //  Light Probe Grid
    // ===========================================================================================

    // Creates a new light probe grid in the scene.
    void create_light_probe_grid(render_object_id id, const char* name);

    // Destroys a light probe grid previously created with create_light_probe_grid
    void destroy_light_probe_grid(render_object_id id);

    // Sets the density of a light probe grid, as a value that represents the seperation between each probe.
    void set_light_probe_grid_density(render_object_id id, float density);

    // Gets a list of all active light probe grid
    std::vector<render_light_probe_grid*> get_light_probe_grids();

    // ===========================================================================================
    //  Reflection Probe
    // ===========================================================================================

    // Creates a new reflection probe in the scene.
    void create_reflection_probe(render_object_id id, const char* name);

    // Destroys a reflection probe previously created with create_reflection_probe
    void destroy_reflection_probe(render_object_id id);

    // Gets a list of all active reflection probes.
    std::vector<render_reflection_probe*> get_reflection_probes();

private:

    friend class render_object;

private:
    renderer& m_renderer;

    std::recursive_mutex m_mutex;

    std::unordered_map<render_object_id, std::unique_ptr<render_object>> m_objects;
    std::vector<render_view*> m_active_views;
    std::vector<render_world*> m_active_worlds;
    std::vector<render_static_mesh*> m_active_static_meshes;
    std::vector<render_directional_light*> m_active_directional_lights;
    std::vector<render_point_light*> m_active_point_lights;
    std::vector<render_spot_light*> m_active_spot_lights;
    std::vector<render_light_probe_grid*> m_active_light_probe_grids;
    std::vector<render_reflection_probe*> m_active_reflection_probes;

};

}; // namespace ws
