// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/render_object.h"
#include "workshop.renderer/assets/model/model.h"
#include "workshop.assets/asset_manager.h"

namespace ws {

// ================================================================================================
//  Represets a static non-animated mesh within the scene.
// ================================================================================================
class render_static_mesh : public render_object
{
public:

    asset_ptr<model> model;

};

}; // namespace ws
