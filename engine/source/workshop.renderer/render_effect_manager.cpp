// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/renderer.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.renderer/assets/shader/shader_loader.h"
#include "workshop.core/filesystem/virtual_file_system.h"

#include "workshop.core/hashing/hash.h"

namespace ws {
    
render_effect_manager::render_effect_manager(renderer& render, asset_manager& asset_manager)
    : m_renderer(render)
    , m_asset_manager(asset_manager)
{
}

void render_effect_manager::register_init(init_list& list)
{
    list.add_step(
        "Load Shaders",
        [this, &list]() -> result<void> { return create_shaders(); },
        [this, &list]() -> result<void> { return destroy_shaders(); }
    );
}

result<void> render_effect_manager::create_shaders()
{
    // Queue all shader assets for load.
    std::vector<std::string> potential_files = virtual_file_system::get().list("data:shaders", virtual_file_system_path_type::file);
    for (std::string& file : potential_files)
    {
        std::string extension = virtual_file_system::get_extension(file.c_str());
        if (extension == asset_manager::k_asset_extension)
        {
            m_shader_assets.push_back(m_asset_manager.request_asset<shader>(file.c_str(), 0));
        }
    }

    // Wait for all loads to complete.
    for (auto& shader_ptr : m_shader_assets)
    {
        shader_ptr.wait_for_load();
        if (shader_ptr.get_state() == asset_loading_state::failed)
        {
            db_fatal(renderer, "Failed to load required shader: %s", shader_ptr.get_path().c_str());
        }
    }

    return true;
}

result<void> render_effect_manager::destroy_shaders()
{
    // Dispose of our references to the shader and wait for the asset manager to drain of disposal requests.
    m_shader_assets.clear();
    m_asset_manager.drain_queue();

    db_assert_message(m_effects.empty(), "Resource leak, destroying register but all effects have not been unregistered.");

    return true;
}

render_effect_manager::effect_id render_effect_manager::register_effect(std::unique_ptr<render_effect>&& effect)
{
    std::scoped_lock lock(m_resource_mutex);

    effect_id id = m_id_counter++;
    m_effects[id] = std::move(effect);
    return id;
}

void render_effect_manager::unregister_effect(effect_id id)
{
    std::scoped_lock lock(m_resource_mutex);

    // TODO: Handle if the effect is in use.

    if (auto iter = m_effects.find(id); iter != m_effects.end())
    {
        m_effects.erase(iter);
    }
}

render_effect* render_effect_manager::get_effect(effect_id id)
{
    std::scoped_lock lock(m_resource_mutex);

    if (auto iter = m_effects.find(id); iter != m_effects.end())
    {
        return iter->second.get();
    }
    return nullptr;
}

render_effect::technique* render_effect_manager::get_technique(const char* name, const std::unordered_map<std::string, std::string>& parameters)
{
    std::scoped_lock lock(m_resource_mutex);

    // Start with list of all techniques.
    std::vector<render_effect::technique*> techniques;

    // Get a list of all potential techniques for matching effect names. We can have
    // multiple effects with the same name, but different techniques (eg. the game extending
    // an effect defined in the engine).
    for (auto& pair : m_effects)
    {
        if (pair.second->name == name)
        {
            for (auto& technique : pair.second->techniques)
            {
                techniques.push_back(technique.get());
            }
        }
    }

    // Remove all techniques that do not accept the parameters we are passing.
    auto iter = std::remove_if(techniques.begin(), techniques.end(), [&parameters](const render_effect::technique* technique) -> bool {

        bool valid = true;

        // Go through all expected parameters.
        for (auto& expected_value : parameters)
        {
            // Find parameter in the techniques variation list.
            for (const render_effect::variation_parameter& param : technique->variation_parameters)
            {
                if (param.name == expected_value.first)
                {
                    // Check the value provided is in the list of accepted values.
                    valid = (std::find(param.values.begin(), param.values.end(), expected_value.second) != param.values.end());
                    break;
                }
            }

            if (!valid)
            {
                valid = false;
                break;
            }
        }

        return !valid;

    });

    techniques.erase(iter, techniques.end());

    // If we have none left we have failed.
    if (techniques.size() == 0)
    {
        return nullptr;
    }
    // If we have one left, we are done.
    else if (techniques.size() == 1)
    {
        return techniques[0];
    }
    // Otherwise we don't have enough information to disambiguate the correct
    // effect so provide an error log to track things down.
    else
    {
        db_warning(renderer, "Attempt to find technique for effect '%s' with the following parameters:", name);
        for (auto& pair : parameters)
        {
            db_warning(renderer, "\t%s = %s", pair.first.c_str(), pair.second.c_str());
        }

        db_warning(renderer, "Provided ambiguous results. Could select any of the following:");
        for (render_effect::technique* instance : techniques)
        {
            db_warning(renderer, "\t%s", instance->name.c_str());
            for (auto& pair : instance->variation_parameters)
            {
                db_warning(renderer, "\t\t%s = [%s]", pair.name.c_str(), string_join(pair.values, ", ").c_str());
            }
        }
        
        return nullptr;
    }
}

void render_effect_manager::swap_effect(effect_id id, effect_id other_id)
{
    std::scoped_lock lock(m_resource_mutex);

    render_effect* effect_1 = m_effects[id].get();
    render_effect* effect_2 = m_effects[other_id].get();

    effect_1->swap(effect_2);
}

}; // namespace ws
