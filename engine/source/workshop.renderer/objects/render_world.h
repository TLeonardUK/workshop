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
//  Represets an individual world within the renderers view of the scene.
//  Worlds are used to segregate which objects are rendered on which viws.
// ================================================================================================
class render_world : public render_object
{
public:
    render_world(render_object_id id, renderer& renderer);
    virtual ~render_world();

};

}; // namespace ws
