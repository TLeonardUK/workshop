// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/assets/scene/scene.h"
#include "workshop.assets/asset_manager.h"

namespace ws {

scene::scene(asset_manager& ass_manager, engine* engine)
    : m_asset_manager(ass_manager)
    , m_engine(engine)
{
}

scene::~scene()
{
}

bool scene::post_load()
{

    return true;
}

}; // namespace ws
