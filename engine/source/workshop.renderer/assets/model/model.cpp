// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/assets/model/model.h"
#include "workshop.renderer/assets/material/material.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_effect.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"

namespace ws {

model::model(ri_interface& ri_interface, renderer& renderer, asset_manager& ass_manager)
    : m_ri_interface(ri_interface)
    , m_renderer(renderer)
    , m_asset_manager(ass_manager)
{
}

model::~model()
{
}

bool model::post_load()
{
    for (material_info& mat : materials)
    {
        mat.material = m_asset_manager.request_asset<material>(mat.file.c_str(), 0);
    }

    return true;
}

}; // namespace ws
