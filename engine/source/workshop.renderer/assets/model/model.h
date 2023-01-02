// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.assets/asset.h"
#include "workshop.core/containers/string.h"

#include "workshop.render_interface/ri_types.h"

#include "workshop.renderer/renderer.h"
#include <array>
#include <unordered_map>

namespace ws {

class asset;
class ri_interface;
class renderer;

// ================================================================================================
//  Model assets respresent all the vertex/index/material references required to render
//  a mesh to the scene.
// ================================================================================================
class model : public asset
{
public:

public:
    model(ri_interface& ri_interface, renderer& renderer);
    virtual ~model();

public:

protected:
    virtual bool post_load() override;

private:
    ri_interface& m_ri_interface;
    renderer& m_renderer;

};

}; // namespace workshop
