// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"
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

private:

    // Gets a pointer to a render object from its id, returns nullptr on failure.
    render_object* resolve_id(render_object_id id);

    renderer& m_renderer;

    std::unordered_map<render_object_id, std::unique_ptr<render_object>> m_objects;
    std::vector<render_view*> m_active_views;
    std::vector<render_static_mesh*> m_active_static_meshes;

};

}; // namespace ws
