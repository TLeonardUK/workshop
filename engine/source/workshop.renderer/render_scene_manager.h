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

    // Updates the visibility of all objects in the scene with respect to all render views.
    void update_visibility();

    // Draw debug bounds of octtree cells.
    void draw_cell_bounds(bool draw_cell_bounds, bool draw_object_bounds);

    // Gets a pointer to a render object from its id, returns nullptr on failure.
    render_object* resolve_id(render_object_id id);

    // Gets a pointer to a render object from its id, returns nullptr on failure.
    template<typename T>
    T* resolve_id_typed(render_object_id id)
    {
        return dynamic_cast<T*>(resolve_id(id));
    }

public:

    // ===========================================================================================
    //  Objects
    // ===========================================================================================

    // Sets the local-space transform of an object within the render scene.
    void set_object_transform(render_object_id id, const vector3& location, const quat& rotation, const vector3& scale);

    // ===========================================================================================
    //  Views
    // ===========================================================================================

    // Creates a new view that has the given id. id's are expected to be unique.
    void create_view(render_object_id id, const char* name);

    // Removes a view previously created with create_view.
    void destroy_view(render_object_id id);

    // Set the viewport in pixel space to which to render the view.    
    void set_view_viewport(render_object_id id, const recti& viewport);

    // Sets the projection parameters of the view.
    void set_view_projection(render_object_id id, float fov, float aspect_ratio, float near_clip, float far_clip);

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

private:

    friend class render_object;

    // Called when various states of an object change, responsible
    // for updated the object within various containers.
    void render_object_created(render_object* obj);
    void render_object_destroyed(render_object* obj);
    void render_object_bounds_modified(render_object* obj);

private:

    inline static const vector3 k_octtree_extents = vector3(100000.0f, 100000.0f, 100000.0f);
    inline static const size_t k_octtree_max_depth = 10;

    renderer& m_renderer;

    std::recursive_mutex m_mutex;

    std::vector<size_t> m_free_view_visibility_indices;

    oct_tree<render_object*> m_object_oct_tree;

    std::unordered_map<render_object_id, std::unique_ptr<render_object>> m_objects;
    std::vector<render_view*> m_active_views;
    std::vector<render_static_mesh*> m_active_static_meshes;
    std::vector<render_directional_light*> m_active_directional_lights;
    std::vector<render_point_light*> m_active_point_lights;
    std::vector<render_spot_light*> m_active_spot_lights;

};

}; // namespace ws
