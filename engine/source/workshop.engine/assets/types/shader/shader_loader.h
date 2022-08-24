// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/assets/asset_loader.h"

namespace ws {

class asset;

// ================================================================================================
//  Loads shaders files.
// 
//  Shader files contain a description of the param blocks, render state, techniques
//  and other associated rendering data required to use a shader as part of
//  a render pass. It is not just a shader on its own.
// ================================================================================================
class shader_loader : public asset_loader
{
public:
    virtual const std::type_info& get_type() override;
    virtual asset* get_default_asset() override;
    virtual asset* load(const char* path) override;
    virtual void unload(asset* instance) override;

};

}; // namespace workshop
