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
//  Material assets bind together all of the neccessary textures, samplers and properties required
//  to render a mesh.
// ================================================================================================
class material : public asset
{
public:

public:
    material(ri_interface& ri_interface, renderer& renderer);
    virtual ~material();

public:

protected:
    virtual bool post_load() override;

private:
    ri_interface& m_ri_interface;
    renderer& m_renderer;

};

}; // namespace workshop
