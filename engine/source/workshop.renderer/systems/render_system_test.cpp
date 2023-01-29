// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_test.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/render_pass_fullscreen.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.renderer/assets/texture/texture.h"
#include "workshop.renderer/assets/material/material.h"
#include "workshop.renderer/assets/model/model.h"

namespace ws {

render_system_test::render_system_test(renderer& render, asset_manager& asset_manager)
    : render_system(render, "test")
    , m_asset_manager(asset_manager)
{
}

void render_system_test::register_init(init_list& list)
{
}

void render_system_test::create_graph(render_graph& graph)
{
    m_test_texture = m_asset_manager.request_asset<texture>("data:tests/test_texture.yaml", 0);
    asset_ptr<material> test_material = m_asset_manager.request_asset<material>("data:tests/test_material.yaml", 0);

    m_asset_manager.drain_queue();


    ri_sampler::create_params sampler_params;
    m_test_sampler = m_renderer.get_render_interface().create_sampler(sampler_params, "test sampler");

    m_test_params = m_renderer.get_param_block_manager().create_param_block("test_params");
    m_test_params->set("albedo_texture", *m_test_texture->ri_instance);
    m_test_params->set("albedo_sampler", *m_test_sampler);

    std::unique_ptr<render_pass_fullscreen> pass = std::make_unique<render_pass_fullscreen>();
    pass->name = "test";
    pass->technique = m_renderer.get_effect_manager().get_technique("test", {});
    pass->output = m_renderer.get_gbuffer_output();
    pass->param_blocks = { 
        m_renderer.get_gbuffer_param_block(), 
        m_test_params.get()
    };
  
    graph.add_node(std::move(pass));
}

void render_system_test::step(const render_world_state& state)
{
}

}; // namespace ws
