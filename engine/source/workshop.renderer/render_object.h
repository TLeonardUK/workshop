// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/vector3.h"
#include "workshop.core/math/quat.h"
#include "workshop.core/math/matrix4.h"
#include "workshop.core/math/obb.h"
#include "workshop.core/containers/oct_tree.h"
#include "workshop.renderer/common_types.h"
#include "workshop.renderer/render_command_queue.h"

#include <string>
#include <bitset>

namespace ws {

class render_scene_manager;

// ================================================================================================
//  Base class for all types of objects that exist within the render scene - meshes, views, etc.
// ================================================================================================
class render_object
{
public:
    render_object(render_object_id id, render_scene_manager* scene_manager, bool stored_in_octtree = true);
    virtual ~render_object();

public:

    // Simple function called after the constructor to do any setup required that cannot occur in the constructor.
    virtual void init();

    // Gets or sets an arbitrary label used to identify this object in the scene.
    void set_name(const std::string& name);
    std::string get_name();

    // Gets the id of this object as used to reference it via the scene manager.
    render_object_id get_id();

    // Modifies the transform of this object and potentially updates render data.
    virtual void set_local_transform(const vector3& location, const quat& rotation, const vector3& scale);

    // Gets the current transform.
    vector3 get_local_location();
    vector3 get_local_scale();
    quat get_local_rotation();

    matrix4 get_transform();

    // Gets the bounds of this object in world space.
    virtual obb get_bounds();

    // Called when the bounds of an object is modified.
    virtual void bounds_modified();

public:

    // Token representing the cell of the global oct tree this object is contained in.
    oct_tree<render_object*>::token object_tree_token;

    // Gets the last frame this object was visible for the given view
    // TODO: This is chonkier than it should be. We should be able to slim this down reasonably easy.
    std::array<size_t, k_max_render_views> last_visible_frame = {};

protected:

    bool m_store_in_octtree = false;

    render_object_id m_id;

    // Scene manager that owns us.
    render_scene_manager* m_scene_manager;

    // Name of the effect, use for debugging.
    std::string m_name;

    // Transformation.
    quat    m_local_rotation = quat::identity;
    vector3 m_local_location = vector3::zero;
    vector3 m_local_scale = vector3::one;

};

}; // namespace ws
