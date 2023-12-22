// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/assets/model/model_loader.h"
#include "workshop.renderer/assets/model/model.h"
#include "workshop.assets/asset_cache.h"
#include "workshop.core/filesystem/file.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/geometry/geometry.h"

#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_shader_compiler.h"

#include "thirdparty/yamlcpp/include/yaml-cpp/yaml.h"

namespace ws {

namespace {

constexpr const char* k_asset_descriptor_type = "model";
constexpr size_t k_asset_descriptor_minimum_version = 1;
constexpr size_t k_asset_descriptor_current_version = 1;

// Bump if compiled format ever changes.
constexpr size_t k_asset_compiled_version = 77;

};

template<>
inline void stream_serialize(stream& out, model::material_info& mat)
{
    stream_serialize(out, mat.name);
    stream_serialize(out, mat.file);
}

template<>
inline void stream_serialize(stream& out, model::mesh_info& mat)
{
    stream_serialize(out, mat.name);
    stream_serialize(out, mat.bounds);
    stream_serialize(out, mat.material_index);

    stream_serialize(out, mat.min_texel_area);
    stream_serialize(out, mat.max_texel_area);
    stream_serialize(out, mat.avg_texel_area);
    stream_serialize(out, mat.min_world_area);
    stream_serialize(out, mat.max_world_area);
    stream_serialize(out, mat.avg_world_area);
    stream_serialize(out, mat.uv_density);

    stream_serialize_list(out, mat.indices);
}

template<>
inline void stream_serialize(stream& out, geometry& geo)
{
    std::vector<geometry_vertex_stream>& streams = geo.get_vertex_streams();

    stream_serialize(out, geo.bounds);

    stream_serialize_list(out, streams, [&out](geometry_vertex_stream& stream) {
        stream_serialize_enum(out, stream.type);
        stream_serialize_enum(out, stream.data_type);
        stream_serialize(out, stream.element_size);
        stream_serialize_list(out, stream.data);
    });
}

model_loader::model_loader(ri_interface& instance, renderer& renderer, asset_manager& ass_manager)
    : m_ri_interface(instance)
    , m_renderer(renderer)
    , m_asset_manager(ass_manager)
{
}

const std::type_info& model_loader::get_type()
{
    return typeid(model);
}

const char* model_loader::get_descriptor_type()
{
    return k_asset_descriptor_type;
}

asset* model_loader::get_default_asset()
{
    return nullptr;
}

asset* model_loader::load(const char* path)
{
    model* asset = new model(m_ri_interface, m_renderer, m_asset_manager);
    if (!serialize(path, *asset, false))
    {
        delete asset;
        return nullptr;
    }
    return asset;
}

void model_loader::unload(asset* instance)
{
    delete instance;
}

bool model_loader::save(const char* path, model& asset)
{
    return serialize(path, asset, true);
}

bool model_loader::serialize(const char* path, model& asset, bool isSaving)
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

    if (!isSaving)
    {
        asset.geometry = std::make_unique<geometry>();
    }

    stream_serialize_list(*stream, asset.materials);
    stream_serialize_list(*stream, asset.meshes);
    stream_serialize(*stream, *asset.geometry);

    return true;
}

bool model_loader::parse_properties(const char* path, YAML::Node& node, model& asset)
{
    std::string source;
    if (!parse_property(path, "source", node["source"], source, true))
    {
        return false;
    }

    std::string source_node;
    if (!parse_property(path, "source_node", node["source_node"], source_node, false))
    {
        return false;
    }

    vector3 scale = vector3::one;
    YAML::Node scale_node = node["scale"];
    if (scale_node.IsDefined())
    {
        if (scale_node.Type() != YAML::NodeType::Sequence || 
            scale_node.size() != 3 ||
            scale_node[0].Type() != YAML::NodeType::Scalar ||
            scale_node[1].Type() != YAML::NodeType::Scalar ||
            scale_node[2].Type() != YAML::NodeType::Scalar)
        {
            db_error(asset, "[%s] scale node is invalid data type.", path);
            return false;
        }

        scale.x = scale_node[0].as<float>();
        scale.y = scale_node[1].as<float>();
        scale.z = scale_node[2].as<float>();
    }

    asset.geometry = geometry::load(source.c_str(), scale);
    asset.source_node = source_node;
    asset.header.add_dependency(source.c_str());

    if (!asset.geometry)
    {
        db_error(asset, "[%s] failed to load geometry from: %s", path, source.c_str());
        return false;
    }

    // Strip out any meshes we dont' care about.


    return true;
}

bool model_loader::parse_materials(const char* path, YAML::Node& node, model& asset)
{
    YAML::Node this_node = node["materials"];

    if (!this_node.IsDefined())
    {
        return true;
    }

    if (this_node.Type() != YAML::NodeType::Map)
    {
        db_error(asset, "[%s] materials node is invalid data type.", path);
        return false;
    }

    for (auto iter = this_node.begin(); iter != this_node.end(); iter++)
    {
        if (!iter->second.IsScalar())
        {
            db_error(asset, "[%s] material value was not scalar value.", path);
            return false;
        }
        else
        {
            std::string name = iter->first.as<std::string>();
            std::string value = iter->second.as<std::string>();

            // Only add the material if the geometry actual uses it.
            geometry_material* geo_mat = asset.geometry->get_material(name.c_str());
            if (geo_mat != nullptr)
            {
                // Note: Don't add as dependency. We don't want to trigger a rebuild of the
                //       model just because a material has changed.
                //asset.header.add_dependency(value.c_str());

                model::material_info& mat = asset.materials.emplace_back();
                mat.name = name;
                mat.file = value;    

                // Add all the meshes that use this material.
                for (geometry_mesh& mesh : asset.geometry->get_meshes())
                {
                    if (mesh.material_index == geo_mat->index && (asset.source_node.empty() || asset.source_node == mesh.name))
                    {
                        model::mesh_info& info = asset.meshes.emplace_back();
                        info.name = mesh.name;
                        info.material_index = asset.materials.size() - 1;
                        info.indices = mesh.indices;
                        info.bounds = mesh.bounds;
                        info.min_texel_area = mesh.min_texel_area;
                        info.max_texel_area = mesh.max_texel_area;
                        info.avg_texel_area = mesh.avg_texel_area;
                        info.min_world_area = mesh.min_world_area;
                        info.max_world_area = mesh.max_world_area;
                        info.avg_world_area = mesh.avg_world_area;
                        info.uv_density = mesh.uv_density;
                    }
                }
            }
        }
    }

    return true;
}

bool model_loader::parse_mesh_namelist(const char* path, YAML::Node& node, model& asset, const char* key, std::vector<std::string>& output)
{
    YAML::Node this_node = node[key];

    if (!this_node.IsDefined())
    {
        return true;
    }

    if (this_node.Type() != YAML::NodeType::Sequence)
    {
        db_error(asset, "[%s] %s node is invalid data type.", path, key);
        return false;
    }

    for (auto iter = this_node.begin(); iter != this_node.end(); iter++)
    {
        if (!iter->IsScalar())
        {
            db_error(asset, "[%s] %s value was not scalar value.", path, key);
            return false;
        }
        else
        {
            std::string name = iter->as<std::string>();
            output.push_back(name);
        }
    }

    return true;
}

bool model_loader::parse_blacklist(const char* path, YAML::Node& node, model& asset)
{
    std::vector<std::string> mesh_blacklist;
    std::vector<std::string> mesh_whitelist;

    if (!parse_mesh_namelist(path, node, asset, "mesh_blacklist", mesh_blacklist) ||
        !parse_mesh_namelist(path, node, asset, "mesh_whitelist", mesh_whitelist))
    {
        return false;
    }

    std::erase_if(asset.meshes, [&mesh_blacklist, &mesh_whitelist](model::mesh_info& mesh) {

        // Explicitly whitelisted, don't erase.
        if (std::find(mesh_whitelist.begin(), mesh_whitelist.end(), mesh.name) != mesh_whitelist.end())
        {
            return false;
        }

        // Explicitly blacklisted, erase.
        if (std::find(mesh_blacklist.begin(), mesh_blacklist.end(), mesh.name) != mesh_blacklist.end())
        {
            return true;
        }

        // Default depends on if we have an explicit whitelist, if we do then blacklist by defualt, otherwise
        // whitelist by default.
        return mesh_whitelist.empty() ? false : true;

    });

    return true;
}

bool model_loader::parse_file(const char* path, model& asset)
{
    db_verbose(asset, "[%s] Parsing file", path);

    YAML::Node node;
    if (!load_asset_descriptor(path, node, k_asset_descriptor_type, k_asset_descriptor_minimum_version, k_asset_descriptor_current_version))
    {
        return false;
    }

    if (!parse_properties(path, node, asset))
    {
        return false;
    }

    if (!parse_materials(path, node, asset))
    {
        return false;
    }

    if (!parse_blacklist(path, node, asset))
    {
       return false;
    }

    // Calculate the geometry bounds to the meshes we've actually used.
    bool first_mesh = true;
    for (model::mesh_info& info : asset.meshes)
    {
        if (first_mesh)
        {
            asset.geometry->bounds = info.bounds;
            first_mesh = false;
        }
        else
        {
            asset.geometry->bounds = asset.geometry->bounds.combine(info.bounds);
        }
    }

    return true;
}

bool model_loader::compile(const char* input_path, const char* output_path, platform_type asset_platform, config_type asset_config, asset_flags flags)
{
    model asset(m_ri_interface, m_renderer, m_asset_manager);

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

size_t model_loader::get_compiled_version()
{
    return k_asset_compiled_version;
}

void model_loader::hot_reload(asset* instance, asset* new_instance)
{
    model* old_instance_typed = static_cast<model*>(instance);
    model* new_instance_typed = static_cast<model*>(new_instance);

    old_instance_typed->swap(new_instance_typed);
}

std::unique_ptr<pixmap> model_loader::generate_thumbnail(const char* path, size_t size)
{
    model asset(m_ri_interface, m_renderer, m_asset_manager);

    // Parse the source YAML file that defines the shader.
    if (!parse_file(path, asset))
    {
        return nullptr;
    }

    // Get the assets we will need to generate the thumbnail loaded.
    asset_ptr<model> model_asset = m_asset_manager.request_asset<model>(path, 0);
    asset_ptr<model> skybox_asset = m_asset_manager.request_asset<model>("data:models/core/skyboxs/default_skybox.yaml", 0);

    model_asset.wait_for_load();
    skybox_asset.wait_for_load();

    // Lock all textures in the texture streamer so they will be fully streamed in.
    std::vector<texture*> textures;

    for (model::material_info& mat : model_asset->materials)
    {
        if (!mat.material.is_loaded())
        {
            continue;
        }

        for (material::texture_info& tex : mat.material->textures)
        {
            if (!tex.texture.is_loaded())
            {
                continue;
            }

            textures.push_back(tex.texture.get());
            m_renderer.get_texture_streamer().lock_texture(tex.texture.get());
        }
    }

    // Wait until all mips are streamed in.
    while (true)
    {
        bool fully_resident = true;

        for (texture* tex : textures)
        {
            if (!m_renderer.get_texture_streamer().is_texture_fully_resident(tex))
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
    render_object_id world_id;

    size_t start_frame_index = 0;

    std::unique_ptr<pixmap> output = std::make_unique<pixmap>(size, size, pixmap_format::R8G8B8A8_SRGB);

    m_renderer.queue_callback(this, [this, &semaphore, &world_id, &model_id, &view_id, &skybox_id, &light_id, &start_frame_index, &model_asset, &skybox_asset, size, &output]()
    {
        // We're running on the RT so just grab the RT command queue directly, avoids extra latency.
        auto& cmd_queue = m_renderer.get_rt_command_queue(); 

        world_id = cmd_queue.create_world("thumbnail world");

        // Calculate the projected bounds of the model and zoom out to a good position to frame it all.
        sphere sphere_bounds = obb(model_asset->geometry->bounds, matrix4::identity).get_sphere();

        vector3 light_location = vector3(1.0f, -1.0f, -1.0f);
        vector3 light_normal = light_location.normalize();
        quat light_rotation = quat::rotate_to(-light_normal, vector3::forward);

        vector3 view_location = vector3(-1.0f, 1.0f, -1.0f);
        vector3 view_normal = view_location.normalize();
        quat view_rotation = quat::rotate_to(-view_normal, vector3::forward);
        float view_fov = 45.0f;
        float view_distance = 0.1f;

        // TODO: This is hot garbage, there should be a non-iterative way to calculate this.
        while (true)
        {
            view_location = view_normal * view_distance;

            float projected_radius = sphere_bounds.projected_screen_radius(
                view_location,
                view_fov,
                (float)size
            );

            if (projected_radius < (float)size * 0.85f) 
            {
                break;
            }

            view_distance *= 1.05f;
        }

        float max_distance = std::max(10000.0f, view_distance + (sphere_bounds.radius * 0.5f));

        view_id = cmd_queue.create_view("thumbnail_view");
        cmd_queue.set_object_transform(view_id, view_location, view_rotation, vector3::one);
        cmd_queue.set_view_projection(view_id, view_fov, 1.0f, 1.0f, max_distance + 1.0f);
        cmd_queue.set_view_viewport(view_id, recti(0, 0, (int)size, (int)size));
        cmd_queue.set_view_readback_pixmap(view_id, output.get());
        cmd_queue.set_object_world(view_id, world_id);

        start_frame_index = m_renderer.get_frame_index();

        // Create model/skybox/light
        model_id = cmd_queue.create_static_mesh("thumbnail_model");
        cmd_queue.set_static_mesh_model(model_id, model_asset);
        cmd_queue.set_object_transform(model_id, -model_asset->geometry->bounds.get_center(), quat::identity, vector3(1.0f, 1.0f, 1.0f));
        cmd_queue.set_object_world(model_id, world_id);

        skybox_id = cmd_queue.create_static_mesh("thumbnail_skybox_model");
        cmd_queue.set_static_mesh_model(skybox_id, skybox_asset);
        cmd_queue.set_object_transform(skybox_id, vector3::zero, quat::identity, vector3(max_distance, max_distance, max_distance));
        cmd_queue.set_object_world(skybox_id, world_id);

        light_id = cmd_queue.create_directional_light("thumbnail_light");
        cmd_queue.set_object_transform(light_id, light_location, light_rotation, vector3::one);
        cmd_queue.set_object_world(light_id, world_id);
        cmd_queue.set_directional_light_shadow_cascades(light_id, 1);
        cmd_queue.set_light_intensity(light_id, 1.0f);
        cmd_queue.set_light_range(light_id, 10000.0f);
        cmd_queue.set_light_importance_distance(light_id, 10000.0f);

        semaphore.release();
    });

    semaphore.acquire();

    // Wait for render of frame to complete on gpu and data to be copied back to our pixmap.
    m_renderer.wait_for_frame(start_frame_index + m_renderer.get_render_interface().get_pipeline_depth());
    
    // Destroy all objects we created to render the thumbnail.    
    m_renderer.queue_callback(this, [this, &semaphore, &model_id, &view_id, &skybox_id, &light_id, &world_id]()
    {
        // We're running on the RT so just grab the RT command queue directly, avoids extra latency.
        auto& cmd_queue = m_renderer.get_rt_command_queue();

        cmd_queue.destroy_static_mesh(model_id);
        cmd_queue.destroy_static_mesh(skybox_id);
        cmd_queue.destroy_directional_light(light_id);
        cmd_queue.destroy_view(view_id);
        cmd_queue.destroy_world(world_id);

        semaphore.release();
    });

    semaphore.acquire();

    // Unlock all the textures that were previously locked.
    for (texture* tex : textures)
    {
        m_renderer.get_texture_streamer().unlock_texture(tex);
    }

    return output;
}

}; // namespace ws

