// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.assets/asset.h"
#include "workshop.core/containers/string.h"
#include "workshop.core/drawing/pixmap.h"

#include "workshop.render_interface/ri_types.h"

#include "workshop.renderer/renderer.h"
#include <array>
#include <unordered_map>

namespace ws {

class asset;
class ri_interface;
class renderer;
class pixmap;

// ================================================================================================
//  Texture assets represent a single individual multidimensional texture.
// ================================================================================================
class texture : public asset
{
public:
    struct face
    {
        std::string file;
        std::unique_ptr<pixmap> pixmap;
    };

public:
    texture(ri_interface& ri_interface, renderer& renderer);
    virtual ~texture();

public:
    std::vector<face> faces;
    std::vector<uint8_t> raw_data;

protected:
    virtual bool post_load() override;

private:
    ri_interface& m_ri_interface;
    renderer& m_renderer;

};

}; // namespace workshop
