// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.assets/asset.h"
#include "workshop.core/containers/string.h"
#include "workshop.core/geometry/geometry.h"

#include "workshop.render_interface/ri_types.h"

#include "workshop.renderer/assets/material/material.h"

#include "workshop.renderer/renderer.h"
#include <array>
#include <unordered_map>

namespace ws {

class asset;
class ri_interface;
class renderer;
class asset_manager;

// ================================================================================================
//  Model assets respresent all the vertex/index/material references required to render
//  a mesh to the scene.
// ================================================================================================
class model : public asset
{
public:
    struct material_info
    {
        std::string name;
        std::string file;
        asset_ptr<material> material;
    };

public:
    model(ri_interface& ri_interface, renderer& renderer, asset_manager& ass_manager);
    virtual ~model();

public:
    std::vector<material_info> materials;
    std::unique_ptr<geometry> geometry;

protected:
    virtual bool post_load() override;

private:
    ri_interface& m_ri_interface;
    renderer& m_renderer;
    asset_manager& m_asset_manager;

};

}; // namespace workshop
