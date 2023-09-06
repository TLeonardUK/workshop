// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/assets/scene/scene_loader.h"
#include "workshop.engine/assets/scene/scene.h"
#include "workshop.engine/engine/world.h"
#include "workshop.assets/asset_cache.h"
#include "workshop.core/filesystem/file.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/filesystem/ram_stream.h"

#include "workshop.engine/ecs/component.h"
#include "workshop.core/reflection/reflect.h"
#include "workshop.core/reflection/reflect_class.h"
#include "workshop.core/reflection/reflect_field.h"

#include "workshop.core/utils/yaml.h"
#include "workshop.engine/utils/yaml.h"

#include "thirdparty/yamlcpp/include/yaml-cpp/yaml.h"

namespace ws {

namespace {

constexpr const char* k_asset_descriptor_type = "scene";
constexpr size_t k_asset_descriptor_minimum_version = 1;
constexpr size_t k_asset_descriptor_current_version = 1;

// Bump if compiled format ever changes.
constexpr size_t k_asset_compiled_version = 8;

};

template<>
inline void stream_serialize(stream& out, scene::component_info& block)
{
    stream_serialize(out, block.type_name_index);
    stream_serialize(out, block.field_offset);
    stream_serialize(out, block.field_count);
}

template<>
inline void stream_serialize(stream& out, scene::field_info& block)
{
    stream_serialize(out, block.field_name_index);
    stream_serialize(out, block.data_offset);
    stream_serialize(out, block.data_size);
}

template<>
inline void stream_serialize(stream& out, scene::object_info& block)
{
    stream_serialize(out, block.handle);
    stream_serialize(out, block.component_offset);
    stream_serialize(out, block.component_count);
}

scene_loader::scene_loader(asset_manager& ass_manager, engine* engine)
    : m_asset_manager(ass_manager)
    , m_engine(engine)
{
}

const std::type_info& scene_loader::get_type()
{
    return typeid(scene);
}

const char* scene_loader::get_descriptor_type()
{
    return k_asset_descriptor_type;
}

asset* scene_loader::get_default_asset()
{
    return nullptr;
}

asset* scene_loader::load(const char* path)
{
    scene* asset = new scene(m_asset_manager, m_engine);
    if (!serialize(path, *asset, false))
    {
        delete asset;
        return nullptr;
    }
    return asset;
}

void scene_loader::unload(asset* instance)
{
    delete instance;
}

bool scene_loader::save(const char* path, scene& asset)
{
    return serialize(path, asset, true);
}

bool scene_loader::compile(const char* input_path, const char* output_path, platform_type asset_platform, config_type asset_config, asset_flags flags)
{
    scene asset(m_asset_manager, m_engine);

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

size_t scene_loader::get_compiled_version()
{
    return k_asset_compiled_version;
}

bool scene_loader::serialize(const char* path, scene& asset, bool isSaving)
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

    stream_serialize_list(*stream, asset.string_table);
    stream_serialize_list(*stream, asset.objects);
    stream_serialize_list(*stream, asset.components);
    stream_serialize_list(*stream, asset.fields);
    stream_serialize_list(*stream, asset.data);

    return true;
}

bool scene_loader::parse_fields(const char* path, YAML::Node& node, scene& asset, scene::component_info& comp, reflect_class* reflect_type, component* deserialize_component)
{
    YAML::Node this_node = node;

    if (!this_node.IsDefined())
    {
        return true;
    }

    if (this_node.Type() != YAML::NodeType::Map)
    {
        db_error(asset, "[%s] fields node is invalid data type.", path);
        return false;
    }

    comp.field_offset = asset.fields.size();

    for (auto iter = this_node.begin(); iter != this_node.end(); iter++)
    {
        std::string field_name = iter->first.as<std::string>();
        YAML::Node child = iter->second;

        reflect_field* field = reflect_type->find_field(field_name.c_str(), true);
        if (field == nullptr)
        {
            db_error(asset, "[%s] field node '%s' is unknown.", path, field_name.c_str());
            return false;
        }

        comp.field_count++;

        scene::field_info& field_data = asset.fields.emplace_back();
        field_data.field_name_index = asset.intern_string(field_name.c_str());

        // Serialize the field data to a temporary component.
        if (!yaml_serialize_reflect(child, true, deserialize_component, field))
        {
            db_warning(engine, "Failed to load yaml for reflect field '%s::%s'.", reflect_type->get_name(), field->get_name());
            return false;
        }

        // Serialize it to binary and append it into our data array.
        std::vector<uint8_t> data;

        ram_stream stream(data, true);
        if (!stream_serialize_reflect(stream, deserialize_component, field))
        {
            db_warning(engine, "Failed to serialize reflect field '%s::%s'.", reflect_type->get_name(), field->get_name());
            return false;
        }

        // Insert into our assets global field data array.
        field_data.data_offset = asset.data.size();
        field_data.data_size = data.size();
        asset.data.insert(asset.data.end(), data.begin(), data.end());
    }

    return true;
}

bool scene_loader::parse_components(const char* path, YAML::Node& node, scene& asset, scene::object_info& obj)
{
    YAML::Node this_node = node;

    if (!this_node.IsDefined())
    {
        return true;
    }

    if (this_node.Type() != YAML::NodeType::Map)
    {
        db_error(asset, "[%s] components node is invalid data type.", path);
        return false;
    }

    obj.component_offset = asset.components.size();

    for (auto iter = this_node.begin(); iter != this_node.end(); iter++)
    {
        std::string component_name = iter->first.as<std::string>();

        if (!iter->second.IsMap())
        {
            db_error(asset, "[%s] component node '%s' was not map type.", path, component_name.c_str());
            return false;
        }

        obj.component_count++;

        scene::component_info& comp = asset.components.emplace_back();
        comp.type_name_index = asset.intern_string(component_name.c_str());
        
        reflect_class* reflect_type = get_reflect_class(component_name.c_str());
        if (reflect_type == nullptr)
        {
            db_error(asset, "[%s] component node '%s' is of an unknown type.", path, component_name.c_str());
            return false;
        }

        std::unique_ptr<component> instance = std::unique_ptr<component>(static_cast<component*>(reflect_type->create_instance()));

        YAML::Node child = iter->second;
        if (!parse_fields(path, child, asset, comp, reflect_type, instance.get()))
        {
            return false;
        }
    }

    return true;
}

bool scene_loader::parse_objects(const char* path, YAML::Node& node, scene& asset, std::vector<scene::object_info>& objects)
{
    YAML::Node this_node = node["objects"];

    if (!this_node.IsDefined())
    {
        return true;
    }

    if (this_node.Type() != YAML::NodeType::Map)
    {
        db_error(asset, "[%s] objects node is invalid data type.", path);
        return false;
    }

    for (auto iter = this_node.begin(); iter != this_node.end(); iter++)
    {
        size_t object_id = iter->first.as<size_t>();

        if (!iter->second.IsMap())
        {
            db_error(asset, "[%s] object node %zi was not map type.", path, object_id);
            return false;
        }

        scene::object_info& obj = objects.emplace_back();
        obj.handle = object_id;

        YAML::Node child = iter->second;
        if (!parse_components(path, child, asset, obj))
        {
            return false;
        }
    }

    return true;
}

bool scene_loader::parse_file(const char* path, scene& asset)
{
    db_verbose(asset, "[%s] Parsing file", path);

    YAML::Node node;
    if (!load_asset_descriptor(path, node, k_asset_descriptor_type, k_asset_descriptor_minimum_version, k_asset_descriptor_current_version))
    {
        return false;
    }

    if (!parse_objects(path, node, asset, asset.objects))
    {
        return false;
    }

    return true;
}

bool scene_loader::save_uncompiled(const char* path, asset& instance)
{
    std::unique_ptr<stream> stream = virtual_file_system::get().open(path, true);
    if (!stream)
    {
        db_error(asset, "[%s] Failed to open stream to save asset.", path);
        return false;
    }

    scene& scene_asset = static_cast<scene&>(instance);
    object_manager& manager = scene_asset.world_instance->get_object_manager();

    // Serialize everything to yaml.
    YAML::Emitter emitter;
    emitter.SetIndent(4);

    emitter << YAML::Comment("================================================================================================") << YAML::Newline;
    emitter << YAML::Comment(" workshop") << YAML::Newline;
    emitter << YAML::Comment(" Copyright (C) 2023 Tim Leonard") << YAML::Newline;
    emitter << YAML::Comment("================================================================================================") << YAML::Newline;
    emitter << YAML::BeginMap;
    emitter << YAML::Key << "type" << YAML::Value << "scene" << YAML::Newline;
    emitter << YAML::Key << "version" << YAML::Value << k_asset_descriptor_current_version << YAML::Newline;
    emitter << YAML::Key << "objects" << YAML::Newline;
    emitter << YAML::BeginMap;
    for (object obj : manager.get_objects())
    {
        emitter << YAML::Key << (size_t)obj;
        emitter << YAML::BeginMap;

            std::vector<component*> components = manager.get_components(obj);
            for (component* comp : components)
            {
                reflect_class* reflect = get_reflect_class(typeid(*comp));
                emitter << YAML::Key << reflect->get_name();
                emitter << YAML::BeginMap;
                for (reflect_field* field : reflect->get_fields(true))
                {
                    emitter << YAML::Key << field->get_name();

                    YAML::Node node;
                    if (!yaml_serialize_reflect(node, false, comp, field))
                    {
                        db_warning(engine, "Failed to emit yaml for reflect field '%s::%s'.", reflect->get_name(), field->get_name());
                        return false;
                    }

                    emitter << node;
                }
                emitter << YAML::EndMap;
            }

        emitter << YAML::EndMap;
    }
    emitter << YAML::EndMap;
    emitter << YAML::EndMap;

    std::string result = emitter.c_str();
    stream->write(result.c_str(), result.size());
    
    return true;
}

}; // namespace ws

