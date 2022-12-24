// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.renderer/render_effect.h"

#include <unordered_map>

namespace ws {

class renderer;
class asset_manager;
class shader;

// ================================================================================================
//  Acts as a central repository for all loaded effects.
// ================================================================================================
class render_effect_manager
{
public:

    // Identifies an effect type that has been registered to the renderer. 
    using effect_id = size_t;

    // Invalid id value for any of the above.
    constexpr static inline size_t invalid_id = 0;

public:

    render_effect_manager(renderer& render, asset_manager& asset_manager);

    // Registers all the steps required to initialize the system.
    void register_init(init_list& list);

public:

    // Registers an effect for use by the renderer. Ownership is transfered to renderer.
    effect_id register_effect(std::unique_ptr<render_effect>&& effect);

    // Unregisters a previous registered effect. This may not take immediately effect
    // if parts of the renderer are still using the effect.
    void unregister_effect(effect_id id);

    // Gets an effect from its id.
    render_effect* get_effect(effect_id id);

    // Gets a renderer technique by its name and a set of parameter values.
    render_effect::technique* get_technique(const char* name, const std::unordered_map<std::string, std::string>& parameters);

private:

    result<void> create_shaders();
    result<void> destroy_shaders();

private:

    std::mutex m_resource_mutex;

    size_t m_id_counter = 1;
    std::unordered_map<effect_id, std::unique_ptr<render_effect>> m_effects;
    std::vector<asset_ptr<shader>> m_shader_assets;

    asset_manager& m_asset_manager;
    renderer& m_renderer;

};

}; // namespace ws
