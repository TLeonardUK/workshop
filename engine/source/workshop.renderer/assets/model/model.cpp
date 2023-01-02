// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/assets/model/model.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_effect.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"

namespace ws {

model::model(ri_interface& ri_interface, renderer& renderer)
    : m_ri_interface(ri_interface)
    , m_renderer(renderer)
{
}

model::~model()
{
}

bool model::post_load()
{
    return true;
}

}; // namespace ws
