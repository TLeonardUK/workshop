// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/render_system.h"
#include "workshop.renderer/assets/texture/texture.h"

namespace ws {

class renderer;
class asset_manager;

// ================================================================================================
//  This is just a test system for experimentation while getting things setup.
//  This should be deleted shortly.
// ================================================================================================
class render_system_test
    : public render_system
{
public:
    render_system_test(renderer& render, asset_manager& asset_manager);

    virtual void register_init(init_list& list) override;
    virtual void create_graph(render_graph& graph) override;
    virtual void step(const render_world_state& state) override;

private:
    asset_manager& m_asset_manager;
    asset_ptr<texture> m_test_texture;
    std::unique_ptr<ri_sampler> m_test_sampler;
    std::unique_ptr<ri_param_block> m_test_params;

};

}; // namespace ws
