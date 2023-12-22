// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/assets/material/material_loader.h"
#include "workshop.renderer/assets/material/material.h"
#include "workshop.renderer/assets/model/model.h"
#include "workshop.assets/asset_cache.h"
#include "workshop.core/filesystem/file.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/filesystem/virtual_file_system.h"

#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_shader_compiler.h"
#include "workshop.render_interface/ri_types.h"

#include "thirdparty/yamlcpp/include/yaml-cpp/yaml.h"

#include <semaphore>

namespace ws {

namespace {

constexpr const char* k_asset_descriptor_type = "material";
constexpr size_t k_asset_descriptor_minimum_version = 1;
constexpr size_t k_asset_descriptor_current_version = 1;

// Bump if compiled format ever changes.
constexpr size_t k_asset_compiled_version = 5;

};

template<>
inline void stream_serialize(stream& out, material::texture_info& block)
{
    stream_serialize(out, block.name);
    stream_serialize(out, block.path);
}

template<>
inline void stream_serialize(stream& out, material::sampler_info& block)
{
    stream_serialize(out, block.name);
    stream_serialize_enum(out, block.filter);
    stream_serialize_enum(out, block.address_mode_u);
    stream_serialize_enum(out, block.address_mode_v);
    stream_serialize_enum(out, block.address_mode_w);
    stream_serialize_enum(out, block.border_color);
    stream_serialize(out, block.min_lod);
    stream_serialize(out, block.max_lod);
    stream_serialize(out, block.mip_lod_bias);
    stream_serialize(out, block.max_anisotropy);
}

template<>
inline void stream_serialize(stream& out, material::parameter_info& block)
{
    stream_serialize(out, block.name);
    stream_serialize(out, block.value);
}

material_loader::material_loader(ri_interface& instance, renderer& renderer, asset_manager& ass_manager)
    : m_ri_interface(instance)
    , m_renderer(renderer)
    , m_asset_manager(ass_manager)
{
}

const std::type_info& material_loader::get_type()
{
    return typeid(material);
}

const char* material_loader::get_descriptor_type()
{
    return k_asset_descriptor_type;
}

asset* material_loader::get_default_asset()
{
    return nullptr;
}

asset* material_loader::load(const char* path)
{
    material* asset = new material(m_ri_interface, m_renderer, m_asset_manager);
    if (!serialize(path, *asset, false))
    {
        delete asset;
        return nullptr;
    }
    return asset;
}

void material_loader::unload(asset* instance)
{
    delete instance;
}

bool material_loader::save(const char* path, material& asset)
{
    return serialize(path, asset, true);
}

bool material_loader::serialize(const char* path, material& asset, bool isSaving)
{
    std::unique_ptr<stream> stream = virtual_file_system::get().open(path, isSaving);
    if (!stream)
    {
        db_error(asset, "[%s] Failed to open stream to save asset.", path);
        return false;
    }

    if (!isSaving)
    {
        asset.header.type = k_asset_descriptor_type;
        asset.header.version = k_asset_compiled_version;
        asset.name = path;
    }

    if (!serialize_header(*stream, asset.header, path))
    {
        return false;
    }

    stream_serialize_enum(*stream, asset.domain);
    stream_serialize_list(*stream, asset.textures);
    stream_serialize_list(*stream, asset.samplers);
    stream_serialize_list(*stream, asset.parameters);

    return true;
}

bool material_loader::parse_textures(const char* path, YAML::Node& node, material& asset)
{
    YAML::Node this_node = node["textures"];

    if (!this_node.IsDefined())
    {
        return true;
    }

    if (this_node.Type() != YAML::NodeType::Map)
    {
        db_error(asset, "[%s] textures node is invalid data type.", path);
        return false;
    }

    for (auto iter = this_node.begin(); iter != this_node.end(); iter++)
    {
        if (!iter->second.IsScalar())
        {
            db_error(asset, "[%s] texture value was not scalar value.", path);
            return false;
        }
        else
        {
            std::string name = iter->first.as<std::string>();
            std::string value = iter->second.as<std::string>();

            // Note: Don't add texture as a dependency. We don't need to rebuild the material asset
            // if only a texture is changed, the texture are independent.
            //asset.header.add_dependency(value.c_str());

            material::texture_info& texture = asset.textures.emplace_back();
            texture.name = name;
            texture.path = value;
        }
    }

    return true;
}

bool material_loader::parse_samplers(const char* path, YAML::Node& node, material& asset)
{
    YAML::Node this_node = node["samplers"];

    if (!this_node.IsDefined())
    {
        return true;
    }

    if (this_node.Type() != YAML::NodeType::Map)
    {
        db_error(asset, "[%s] samplers node is invalid data type.", path);
        return false;
    }

    for (auto iter = this_node.begin(); iter != this_node.end(); iter++)
    {
        std::string name = iter->first.as<std::string>();

        if (!iter->second.IsMap())
        {
            db_error(asset, "[%s] sampler node '%s' was not map type.", path, name.c_str());
            return false;
        }

        YAML::Node child = iter->second;
        if (!parse_sampler(path, name.c_str(), child, asset))
        {
            return false;
        }
    }

    return true;
}

bool material_loader::parse_sampler(const char* path, const char* name, YAML::Node& node, material& asset)
{
    material::sampler_info& sampler = asset.samplers.emplace_back();
    sampler.name = name;

    if (!parse_property(path, "filter", node["filter"], sampler.filter, false))
    {
        return false;
    }

    if (!parse_property(path, "address_mode_u", node["address_mode_u"], sampler.address_mode_u, false))
    {
        return false;
    }

    if (!parse_property(path, "address_mode_v", node["address_mode_v"], sampler.address_mode_v, false))
    {
        return false;
    }

    if (!parse_property(path, "address_mode_w", node["address_mode_w"], sampler.address_mode_w, false))
    {
        return false;
    }

    if (!parse_property(path, "border_color", node["border_color"], sampler.border_color, false))
    {
        return false;
    }

    if (!parse_property(path, "min_lod", node["min_lod"], sampler.min_lod, false))
    {
        return false;
    }

    if (!parse_property(path, "max_lod", node["max_lod"], sampler.max_lod, false))
    {
        return false;
    }

    if (!parse_property(path, "mip_lod_bias", node["mip_lod_bias"], sampler.mip_lod_bias, false))
    {
        return false;
    }

    if (!parse_property(path, "max_anisotropy", node["max_anisotropy"], sampler.max_anisotropy, false))
    {
        return false;
    }

    return true;
}

bool material_loader::parse_parameters(const char* path, YAML::Node& node, material& asset)
{
    YAML::Node this_node = node["parameters"];

    if (!this_node.IsDefined())
    {
        return true;
    }

    if (this_node.Type() != YAML::NodeType::Map)
    {
        db_error(asset, "[%s] parameters node is invalid data type.", path);
        return false;
    }

    for (auto iter = this_node.begin(); iter != this_node.end(); iter++)
    {
        if (!iter->second.IsScalar())
        {
            db_error(asset, "[%s] parameter value was not scalar value.", path);
            return false;
        }
        else
        {
            std::string name = iter->first.as<std::string>();
            std::string value = iter->second.as<std::string>();

            material::parameter_info& texture = asset.parameters.emplace_back();
            texture.name = name;
            texture.value = value;
        }
    }

    return true;
}

bool material_loader::parse_file(const char* path, material& asset)
{
    db_verbose(asset, "[%s] Parsing file", path);

    YAML::Node node;
    if (!load_asset_descriptor(path, node, k_asset_descriptor_type, k_asset_descriptor_minimum_version, k_asset_descriptor_current_version))
    {
        return false;
    }

    if (!parse_property(path, "domain", node["domain"], asset.domain, true))
    {
        return false;
    }

    if (!parse_textures(path, node, asset))
    {
        return false;
    }

    if (!parse_samplers(path, node, asset))
    {
        return false;
    }

    if (!parse_parameters(path, node, asset))
    {
        return false;
    }

    return true;
}

bool material_loader::compile(const char* input_path, const char* output_path, platform_type asset_platform, config_type asset_config, asset_flags flags)
{
    material asset(m_ri_interface, m_renderer, m_asset_manager);

    // Parse the source YAML file that defines the shader.
    if (!parse_file(input_path, asset))
    {
        return false;
    }

    // Construct the asset header.
    asset_cache_key compiled_key;
    if (!get_cache_key(input_path, asset_platform, asset_config, flags, compiled_key, asset.header.dependencies))
    {
        db_error(asset, "[%s] Failed to calculate compiled cache key.", input_path);
        return false;
    }
    asset.header.compiled_hash = compiled_key.hash();
    asset.header.type = k_asset_descriptor_type;
    asset.header.version = k_asset_compiled_version;

    // Write binary format to disk.
    if (!save(output_path, asset))
    {
        return false;
    }

    return true;
}

std::unique_ptr<pixmap> material_loader::generate_thumbnail(const char* path, size_t size)
{
    material asset(m_ri_interface, m_renderer, m_asset_manager);

    // Parse the source YAML file that defines the shader.
    if (!parse_file(path, asset))
    {
        return nullptr;
    }

    asset_loader* texture_asset_loader = m_asset_manager.get_loader_for_descriptor_type("texture");
    if (!texture_asset_loader)
    {
        return nullptr;
    }

    // Get the assets we will need to generate the thumbnail loaded.
    asset_ptr<material> material_asset = m_asset_manager.request_asset<material>(path, 0);
    asset_ptr<model> sphere_asset = m_asset_manager.request_asset<model>("data:models/core/primitives/sphere.yaml", 0);
    asset_ptr<model> skybox_asset = m_asset_manager.request_asset<model>("data:models/core/skyboxs/default_skybox.yaml", 0);

    material_asset.wait_for_load();
    sphere_asset.wait_for_load();
    skybox_asset.wait_for_load();

    // Lock all textures in the texture streamer so they will be fully streamed in.
    for (material::texture_info& tex : material_asset->textures)
    {
        if (!tex.texture.is_loaded())
        {
            continue;
        }
        m_renderer.get_texture_streamer().lock_texture(tex.texture.get());
    }

    // Wait until all mips are streamed in.
    while (true)
    {
        bool fully_resident = true;

        for (material::texture_info& tex : material_asset->textures)
        {
            if (!tex.texture.is_loaded())
            {
                continue;
            }

            if (!m_renderer.get_texture_streamer().is_texture_fully_resident(tex.texture.get()))
            {
                fully_resident = false;
                break;
            }
        }
        
        if (fully_resident)
        {
            break;
        }

        m_renderer.wait_for_frame(m_renderer.get_frame_index() + 1);
    }

    // Setup the scene to render out material.
    std::binary_semaphore semaphore { 0 };

    render_object_id model_id;
    render_object_id skybox_id;
    render_object_id light_id;
    render_object_id view_id;

    size_t start_frame_index = 0;

    std::unique_ptr<pixmap> output = std::make_unique<pixmap>(size, size, pixmap_format::R8G8B8A8_SRGB);

    m_renderer.queue_callback(this, [this, &semaphore, &model_id, &view_id, &skybox_id, &light_id, &start_frame_index, &sphere_asset, &material_asset, &skybox_asset, size, &output]()
    {
        // We're running on the RT so just grab the RT command queue directly, avoids extra latency.
        auto& cmd_queue = m_renderer.get_rt_command_queue(); 

        model_id = cmd_queue.create_static_mesh("thumbnail_model");
        cmd_queue.set_static_mesh_model(model_id, sphere_asset);
        cmd_queue.set_static_mesh_materials(model_id, { material_asset });
        cmd_queue.set_object_transform(model_id, vector3::zero, quat::identity, vector3(100.0f, 100.0f, 100.0f));

        skybox_id = cmd_queue.create_static_mesh("thumbnail_skybox_model");
        cmd_queue.set_static_mesh_model(skybox_id, skybox_asset);
        cmd_queue.set_object_transform(skybox_id, vector3::zero, quat::identity, vector3(1000.0f, 1000.0f, 1000.0f));

        light_id = cmd_queue.create_directional_light("thumbnail_light");
        cmd_queue.set_object_transform(light_id, vector3::zero, quat::identity.rotate_x(math::halfpi) * quat::identity.rotate_y(math::halfpi), vector3::one);

        view_id = cmd_queue.create_view("thumbnail_view");        
        cmd_queue.set_object_transform(view_id, vector3(0.0f, 0.0f, -150.0f), quat::identity, vector3::one);
        cmd_queue.set_view_projection(view_id, 45.0f, 1.0f, 1.0f, 10000.0f);
        cmd_queue.set_view_viewport(view_id, recti(0, 0, (int)size, (int)size));
        cmd_queue.set_view_readback_pixmap(view_id, output.get());

        start_frame_index = m_renderer.get_frame_index();

        semaphore.release();
    });

    semaphore.acquire();

    // Wait for render of frame to complete on gpu and data to be copied back to our pixmap.
    m_renderer.wait_for_frame(start_frame_index + m_renderer.get_render_interface().get_pipeline_depth());
    
    // Destroy all objects we created to render the thumbnail.    
    m_renderer.queue_callback(this, [this, &semaphore, &model_id, &view_id, &skybox_id, &light_id]()
    {
        // We're running on the RT so just grab the RT command queue directly, avoids extra latency.
        auto& cmd_queue = m_renderer.get_rt_command_queue();

        cmd_queue.destroy_static_mesh(model_id);
        cmd_queue.destroy_static_mesh(skybox_id);
        cmd_queue.destroy_directional_light(light_id);
        cmd_queue.destroy_view(view_id);

        semaphore.release();
    });

    semaphore.acquire();

    // Unlock all the textures that were previously locked.
    for (material::texture_info& tex : material_asset->textures)
    {
        if (!tex.texture.is_loaded())
        {
            continue;
        }

        m_renderer.get_texture_streamer().unlock_texture(tex.texture.get());
    }

    return output;
}

void material_loader::hot_reload(asset* instance, asset* new_instance)
{
    material* old_instance_typed = static_cast<material*>(instance);
    material* new_instance_typed = static_cast<material*>(new_instance);

    // Invalidate batches so they are recreated for the new material.
    m_renderer.get_batch_manager().clear_cached_material_data(old_instance_typed);

    old_instance_typed->swap(new_instance_typed);
}

size_t material_loader::get_compiled_version()
{
    return k_asset_compiled_version;
}

}; // namespace ws

