// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/assets/types/shader/shader_loader.h"
#include "workshop.engine/assets/types/shader/shader.h"
#include "workshop.engine/assets/asset_cache.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.core/filesystem/file.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/filesystem/virtual_file_system.h"

#include "workshop.render_interface/render_interface.h"
#include "workshop.render_interface/render_shader_compiler.h"

#include "thirdparty/yamlcpp/include/yaml-cpp/yaml.h"

namespace ws {

namespace {

constexpr const char* k_asset_descriptor_type = "shader";
constexpr size_t k_asset_descriptor_minimum_version = 1;
constexpr size_t k_asset_descriptor_current_version = 1;

// Bump if compiled format ever changes.
constexpr size_t k_asset_compiled_version = 1;

};

template<>
inline void stream_serialize(stream& out, shader::param_block& block)
{
    stream_serialize(out, block.name);
    stream_serialize_enum(out, block.scope);

    stream_serialize_list(out, block.fields, [&out](shader::param_block::field& field) {
        stream_serialize(out, field.name);
        stream_serialize_enum(out, field.data_type);
    });
}

template<>
inline void stream_serialize(stream& out, shader::render_state& block)
{
    stream_serialize(out, block.name);
    stream_serialize(out, block.state);
}

template<>
inline void stream_serialize(stream& out, shader::variation& block)
{
    stream_serialize(out, block.name);
    stream_serialize_list(out, block.values);
}

template<>
inline void stream_serialize(stream& out, shader::vertex_layout& block)
{
    stream_serialize(out, block.name);

    stream_serialize_list(out, block.fields, [&out](shader::vertex_layout::field& field) {
        stream_serialize(out, field.name);
        stream_serialize_enum(out, field.data_type);
    });
}

template<>
inline void stream_serialize(stream& out, shader::effect& block)
{
    stream_serialize(out, block.name);

    stream_serialize_list(out, block.techniques, [&out](shader::effect::technique& technique) {
        stream_serialize(out, technique.name);
        stream_serialize_list(out, technique.variations);
    });
}

template<>
inline void stream_serialize(stream& out, shader::technique& block)
{
    stream_serialize(out, block.name);

    for (size_t i = 0; i < block.stages.size(); i++)
    {
        shader::technique::stage& stage = block.stages[i];
        stream_serialize(out, stage.file);
        stream_serialize(out, stage.entry_point);
        stream_serialize_list(out, stage.bytecode);
    }

    stream_serialize(out, block.render_state_index);
    stream_serialize(out, block.vertex_layout_index);
    stream_serialize_list(out, block.param_block_indices);
    stream_serialize_map(out, block.defines);
}

shader_loader::shader_loader(engine& instance)
    : m_engine(instance)
{
}

const std::type_info& shader_loader::get_type()
{
    return typeid(shader);
}

asset* shader_loader::get_default_asset()
{
    return nullptr;
}

asset* shader_loader::load(const char* path)
{
    shader* asset = new shader();
    if (!serialize(path, *asset, false))
    {
        delete asset;
        return nullptr;
    }
    return asset;
}

void shader_loader::unload(asset* instance)
{
    delete instance;
}

bool shader_loader::save(const char* path, shader& asset)
{
    return serialize(path, asset, true);
}

bool shader_loader::serialize(const char* path, shader& asset, bool isSaving)
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
    }

    if (!serialize_header(*stream, asset.header, path))
    {
        return false;
    }
        
    stream_serialize_list(*stream, asset.param_blocks);
    stream_serialize_list(*stream, asset.render_states);
    stream_serialize_list(*stream, asset.variations);
    stream_serialize_list(*stream, asset.vertex_layouts);
    stream_serialize_list(*stream, asset.effects);
    stream_serialize_list(*stream, asset.techniques);

    return true;
}

bool shader_loader::parse_imports(const char* path, YAML::Node& node, shader& asset)
{
    YAML::Node imports_node = node["imports"];

    if (!imports_node.IsDefined())
    {
        return true;
    }

    if (imports_node.Type() != YAML::NodeType::Sequence)
    {
        db_error(asset, "[%s] imports node is invalid data type.", path);
        return false;
    }

    for (auto iter = imports_node.begin(); iter != imports_node.end(); iter++)
    {
        if (!iter->IsScalar())
        {
            db_error(asset, "[%s] imports value was not scalar value.", path);
            return false;
        }
        else
        {
            std::string value = iter->as<std::string>();

            asset.header.add_dependency(value.c_str());

            if (!parse_file(value.c_str(), asset))
            {
                return false;
            }
        }
    }

    return true;
}

bool shader_loader::parse_param_blocks(const char* path, YAML::Node& node, shader& asset)
{
    YAML::Node param_blocks_node = node["param_blocks"];

    if (!param_blocks_node.IsDefined())
    {
        return true;
    }

    if (param_blocks_node.Type() != YAML::NodeType::Map)
    {
        db_error(asset, "[%s] param_blocks node is invalid data type.", path);
        return false;
    }

    for (auto iter = param_blocks_node.begin(); iter != param_blocks_node.end(); iter++)
    {
        std::string name = iter->first.as<std::string>();

        if (!iter->second.IsMap())
        {
            db_error(asset, "[%s] param block node '%s' was not map type.", path, name.c_str());
            return false;
        }

        YAML::Node child = iter->second;
        if (!parse_param_block(path, name.c_str(), child, asset))
        {
            return false;
        }
    }

    return true;
}

bool shader_loader::parse_param_block(const char* path, const char* name, YAML::Node& node, shader& asset)
{
    YAML::Node scope_node = node["scope"];
    YAML::Node fields_node = node["fields"];

    shader::param_block& block = asset.param_blocks.emplace_back();
    block.name = name;

    if (!scope_node.IsDefined())
    {
        db_error(asset, "[%s] scope not defined for param block '%s'.", path, name);
        return false;
    }
    if (scope_node.Type() != YAML::NodeType::Scalar)
    {
        db_error(asset, "[%s] scope for param block '%s' was not a scalar type.", path, name);
        return false;
    }

    if (auto ret = from_string<shader::param_block::scopes>(scope_node.as<std::string>()); ret)
    {
        block.scope = ret.get();
    }
    else
    {
        db_error(asset, "[%s] scope for param block '%s' is invalid type '%s'.", path, scope_node.as<std::string>().c_str());
        return false;
    }

    if (!fields_node.IsDefined())
    {
        db_error(asset, "[%s] fields not defined for param block '%s'.", path, name);
        return false;
    }
    if (fields_node.Type() != YAML::NodeType::Map)
    {
        db_error(asset, "[%s] fields for param block '%s' was not a scalar type.", path, name);
        return false;
    }

    for (auto iter = fields_node.begin(); iter != fields_node.end(); iter++)
    {
        shader::param_block::field& field = block.fields.emplace_back();
        field.name = iter->first.as<std::string>();

        if (!iter->second.IsScalar())
        {
            db_error(asset, "[%s] param block field '%s' was not map type.", path, field.name.c_str());
            return false;
        }

        std::string field_data_type = iter->second.as<std::string>();

        if (auto ret = from_string<render_data_type>(field_data_type.c_str()); ret)
        {
            field.data_type = ret.get();
        }
        else
        {
            db_error(asset, "[%s] param block field '%s' has invalid data type '%s'.", path, field.name.c_str(), field_data_type.c_str());
            return false;
        }
    }

    return true;
}

bool shader_loader::parse_render_states(const char* path, YAML::Node& node, shader& asset)
{
    YAML::Node render_states_node = node["render_states"];

    if (!render_states_node.IsDefined())
    {
        return true;
    }

    if (render_states_node.Type() != YAML::NodeType::Map)
    {
        db_error(asset, "[%s] render_states node is invalid data type.", path);
        return false;
    }

    for (auto iter = render_states_node.begin(); iter != render_states_node.end(); iter++)
    {
        std::string name = iter->first.as<std::string>();

        if (!iter->second.IsMap())
        {
            db_error(asset, "[%s] render state node '%s' was not map type.", path, name.c_str());
            return false;
        }

        YAML::Node child = iter->second;
        if (!parse_render_state(path, name.c_str(), child, asset))
        {
            return false;
        }
    }

    return true;
}

bool shader_loader::parse_render_state(const char* path, const char* name, YAML::Node& node, shader& asset)
{
    bool variables_valid = true;

    auto ReadVariable = [&path, &node, &variables_valid](const char* name, auto& source, auto default_value)
    {
        YAML::Node child_node = node[name];
        if (!child_node.IsDefined())
        {
            source = default_value;
            return;
        }

        std::string child_value = child_node.as<std::string>();
        if (auto ret = from_string<decltype(default_value)>(child_value); ret)
        {
            source = ret.get();
        }
        else
        {
            db_error(asset, "[%s] value for '%s' is invalid '%s'.", path, name, child_value.c_str());
        }
    };

    shader::render_state& block = asset.render_states.emplace_back();
    block.name = name;

    ReadVariable("topology",                        block.state.topology,                           render_topology::triangle);
    ReadVariable("fill_mode",                       block.state.fill_mode,                          render_fill_mode::solid);
    ReadVariable("cull_mode",                       block.state.cull_mode,                          render_cull_mode::back);
    ReadVariable("depth_bias",                      block.state.depth_bias,                         0u);
    ReadVariable("depth_bias_clamp",                block.state.depth_bias_clamp,                   0.0f);
    ReadVariable("slope_scaled_depth_bias",         block.state.slope_scaled_depth_bias,            0.0f);
    ReadVariable("depth_clip_enabled",              block.state.depth_clip_enabled,                 true);
    ReadVariable("multisample_enabled",             block.state.multisample_enabled,                false);
    ReadVariable("antialiased_line_enabled",        block.state.antialiased_line_enabled,           false);
    ReadVariable("conservative_raster_enabled",     block.state.conservative_raster_enabled,        false);

    ReadVariable("alpha_to_coverage",               block.state.alpha_to_coverage,                  false);
    ReadVariable("blend_enabled",                   block.state.blend_enabled,                      false);
    ReadVariable("blend_op",                        block.state.blend_op,                           render_blend_op::add);
    ReadVariable("blend_source_op",                 block.state.blend_source_op,                    render_blend_operand::one);
    ReadVariable("blend_destination_op",            block.state.blend_destination_op,               render_blend_operand::zero);
    ReadVariable("blend_alpha_op",                  block.state.blend_alpha_op,                     render_blend_op::add);
    ReadVariable("blend_alpha_source_op",           block.state.blend_alpha_source_op,              render_blend_operand::one);
    ReadVariable("blend_alpha_destination_op",      block.state.blend_alpha_destination_op,         render_blend_operand::zero);

    ReadVariable("depth_test_enabled",              block.state.depth_test_enabled,                 true);
    ReadVariable("depth_write_enabled",             block.state.depth_write_enabled,                true);
    ReadVariable("depth_compare_op",                block.state.depth_compare_op,                   render_compare_op::less);

    ReadVariable("stencil_test_enabled",            block.state.stencil_test_enabled,               false);
    ReadVariable("stencil_read_mask",               block.state.stencil_read_mask,                  0u);
    ReadVariable("stencil_write_mask",              block.state.stencil_write_mask,                 0u);
    ReadVariable("stencil_front_face_fail_op",      block.state.stencil_front_face_fail_op,         render_stencil_op::keep);
    ReadVariable("stencil_front_face_depth_fail_op",block.state.stencil_front_face_depth_fail_op,   render_stencil_op::keep);
    ReadVariable("stencil_front_face_pass_op",      block.state.stencil_front_face_pass_op,         render_stencil_op::keep);
    ReadVariable("stencil_front_face_compare_op",   block.state.stencil_front_face_compare_op,      render_compare_op::always);
    ReadVariable("stencil_back_face_fail_op",       block.state.stencil_back_face_fail_op,          render_stencil_op::keep);
    ReadVariable("stencil_back_face_depth_fail_op", block.state.stencil_back_face_depth_fail_op,    render_stencil_op::keep);
    ReadVariable("stencil_back_face_pass_op",       block.state.stencil_back_face_pass_op,          render_stencil_op::keep);
    ReadVariable("stencil_back_face_compare_op",    block.state.stencil_back_face_compare_op,       render_compare_op::always);

    if (!variables_valid)
    {
        return false;
    }

    return true;
}

bool shader_loader::parse_variations(const char* path, YAML::Node& node, shader& asset)
{
    YAML::Node variations_node = node["variations"];

    if (!variations_node.IsDefined())
    {
        return true;
    }

    if (variations_node.Type() != YAML::NodeType::Map)
    {
        db_error(asset, "[%s] variations node is invalid data type.", path);
        return false;
    }

    for (auto iter = variations_node.begin(); iter != variations_node.end(); iter++)
    {
        std::string name = iter->first.as<std::string>();

        if (!iter->second.IsSequence())
        {
            db_error(asset, "[%s] variation node '%s' was not sequence type.", path, name.c_str());
            return false;
        }

        YAML::Node child = iter->second;
        if (!parse_variation(path, name.c_str(), child, asset.variations))
        {
            return false;
        }
    }

    return true;
}

bool shader_loader::parse_variation(const char* path, const char* name, YAML::Node& node, std::vector<shader::variation>& variations)
{
    shader::variation& block = variations.emplace_back();
    block.name = name;

    for (auto iter = node.begin(); iter != node.end(); iter++)
    {
        if (!iter->IsScalar())
        {
            db_error(asset, "[%s] variation value for '%s' was not scalar type.", path, name);
            return false;
        }

        block.values.push_back(iter->as<std::string>());
    }

    return true;
}

bool shader_loader::parse_vertex_layouts(const char* path, YAML::Node& node, shader& asset)
{
    YAML::Node vertex_layouts_node = node["vertex_layouts"];

    if (!vertex_layouts_node.IsDefined())
    {
        return true;
    }

    if (vertex_layouts_node.Type() != YAML::NodeType::Map)
    {
        db_error(asset, "[%s] vertex_layouts node is invalid data type.", path);
        return false;
    }

    for (auto iter = vertex_layouts_node.begin(); iter != vertex_layouts_node.end(); iter++)
    {
        std::string name = iter->first.as<std::string>();

        if (!iter->second.IsMap())
        {
            db_error(asset, "[%s] vertex layout node '%s' was not map type.", path, name.c_str());
            return false;
        }

        YAML::Node child = iter->second;
        if (!parse_vertex_layout(path, name.c_str(), child, asset))
        {
            return false;
        }
    }

    return true;
}

bool shader_loader::parse_vertex_layout(const char* path, const char* name, YAML::Node& node, shader& asset)
{
    shader::vertex_layout& block = asset.vertex_layouts.emplace_back();
    block.name = name;

    for (auto iter = node.begin(); iter != node.end(); iter++)
    {
        shader::vertex_layout::field& field = block.fields.emplace_back();
        field.name = iter->first.as<std::string>();

        if (!iter->second.IsScalar())
        {
            db_error(asset, "[%s] vertex layout field '%s' was not map type.", path, field.name.c_str());
            return false;
        }

        std::string field_data_type = iter->second.as<std::string>();
        if (auto ret = from_string<render_data_type>(field_data_type.c_str()); ret)
        {
            field.data_type = ret.get();
        }
        else
        {
            db_error(asset, "[%s] vertex layout field '%s' has invalid data type '%s'.", path, field.name.c_str(), field_data_type.c_str());
            return false;
        }
    }

    return true;
}

bool shader_loader::parse_techniques(const char* path, YAML::Node& node, shader& asset)
{
    YAML::Node techniques_node = node["techniques"];

    if (!techniques_node.IsDefined())
    {
        return true;
    }

    if (techniques_node.Type() != YAML::NodeType::Map)
    {
        db_error(asset, "[%s] techniques node is invalid data type.", path);
        return false;
    }

    for (auto iter = techniques_node.begin(); iter != techniques_node.end(); iter++)
    {
        std::string name = iter->first.as<std::string>();

        if (!iter->second.IsMap())
        {
            db_error(asset, "[%s] technique node '%s' was not map type.", path, name.c_str());
            return false;
        }

        YAML::Node child = iter->second;
        if (!parse_technique(path, name.c_str(), child, asset))
        {
            return false;
        }
    }

    return true;
}

bool shader_loader::parse_technique(const char* path, const char* name, YAML::Node& node, shader& asset)
{
    std::array<YAML::Node, static_cast<int>(render_shader_stage::COUNT)> stage_nodes = {
        node["vertex_shader"],
        node["pixel_shader"],
        node["domain_shader"],
        node["hull_shader"],
        node["geometry_shader"],
        node["compute_shader"]
    };

    YAML::Node render_state_node = node["render_state"];
    YAML::Node vertex_layout_node = node["vertex_layout"];
    YAML::Node param_blocks_node = node["param_blocks"];
    YAML::Node defines_node = node["defines"];

    shader::technique& block = asset.techniques.emplace_back();
    block.name = name;

    // Parse shader stages.
    size_t shader_count = 0;
    for (size_t i = 0; i < stage_nodes.size(); i++)
    {
        render_shader_stage stage = static_cast<render_shader_stage>(i);
        YAML::Node stage_node = stage_nodes[i];

        if (!stage_node.IsDefined())
        {
            continue;
        }
        if (stage_node.Type() != YAML::NodeType::Map)
        {
            db_error(asset, "[%s] shader stage node for technique '%s' was not a map type.", path, name);
            return false;
        }

        YAML::Node file_node = stage_node["file"];
        YAML::Node entry_node = stage_node["entry"];

        if (!file_node.IsDefined())
        {
            db_error(asset, "[%s] shader stage node for technique '%s' does not have a file value.", path, name);
            return false;
        }
        if (file_node.Type() != YAML::NodeType::Scalar)
        {
            db_error(asset, "[%s] shader stage file value for technique '%s' was not a scalar type.", path, name);
            return false;
        }

        if (!entry_node.IsDefined())
        {
            db_error(asset, "[%s] shader stage node for technique '%s' does not have a entry value.", path, name);
            return false;
        }
        if (entry_node.Type() != YAML::NodeType::Scalar)
        {
            db_error(asset, "[%s] shader stage entry value for technique '%s' was not a scalar type.", path, name);
            return false;
        }

        block.stages[i].entry_point = entry_node.as<std::string>();
        block.stages[i].file = file_node.as<std::string>();
        shader_count++;
    }

    if (shader_count == 0)
    {
        db_error(asset, "[%s] technique '%s' defines no shader stages.", path, name);
        return false;
    }

    // Parse render state.
    if (!render_state_node.IsDefined())
    {
        db_error(asset, "[%s] technique '%s' has no defined render state.", path, name);
        return false;
    }

    if (render_state_node.Type() != YAML::NodeType::Scalar)
    {
        db_error(asset, "[%s] render state for technique '%s' was not a scalar type.", path, name);
        return false;
    }

    std::string render_state_name = render_state_node.as<std::string>();
    auto render_state_iter = std::find_if(asset.render_states.begin(), asset.render_states.end(), [&render_state_name](const shader::render_state& state) {
        return state.name == render_state_name;
    });
    if (render_state_iter == asset.render_states.end())
    {
        db_error(asset, "[%s] render state '%s' for technique '%s' was not found.", path, render_state_name.c_str(), name);
        return false;                
    }
    else
    {
        block.render_state_index = std::distance(asset.render_states.begin(), render_state_iter);
    }

    // Parse vertex layout.
    if (!vertex_layout_node.IsDefined())
    {
        db_error(asset, "[%s] technique '%s' has no defined vertex layout.", path, name);
        return false;
    }

    if (vertex_layout_node.Type() != YAML::NodeType::Scalar)
    {
        db_error(asset, "[%s] vertex layout for technique '%s' was not a scalar type.", path, name);
        return false;
    }
    
    std::string vertex_layout_name = vertex_layout_node.as<std::string>();
    auto vertex_layout_iter = std::find_if(asset.vertex_layouts.begin(), asset.vertex_layouts.end(), [&vertex_layout_name](const shader::vertex_layout& state) {
        return state.name == vertex_layout_name;
    });
    if (vertex_layout_iter == asset.vertex_layouts.end())
    {
        db_error(asset, "[%s] vertex layout '%s' for technique '%s' was not found.", path, vertex_layout_name.c_str(), name);
        return false;                
    }
    else
    {
        block.vertex_layout_index = std::distance(asset.vertex_layouts.begin(), vertex_layout_iter);
    }

    // Parse param blocks.
    if (param_blocks_node.IsDefined())
    {
        if (param_blocks_node.Type() != YAML::NodeType::Sequence)
        {
            db_error(asset, "[%s] param blocks for technique '%s' was not a sequence type.", path, name);
            return false;
        }

        for (auto iter = param_blocks_node.begin(); iter != param_blocks_node.end(); iter++)
        {
            if (!iter->IsScalar())
            {
                db_error(asset, "[%s] param block value for technique '%s' was not scalar type.", path, name);
                return false;
            }
            
            std::string param_block_name = iter->as<std::string>();
            auto param_block_iter = std::find_if(asset.param_blocks.begin(), asset.param_blocks.end(), [&param_block_name](const shader::param_block& state) {
                return state.name == param_block_name;
            });
            if (param_block_iter == asset.param_blocks.end())
            {
                db_error(asset, "[%s] param block '%s' for technique '%s' was not found.", path, param_block_name.c_str(), name);
                return false;                
            }
            else
            {
                block.param_block_indices.push_back(std::distance(asset.param_blocks.begin(), param_block_iter));
            }
        }
    }

    // Prase defines.
    if (defines_node.IsDefined())
    {
        if (defines_node.Type() != YAML::NodeType::Map)
        {
            db_error(asset, "[%s] defines blocks for technique '%s' was not a map type.", path, name);
            return false;
        }

        for (auto iter = defines_node.begin(); iter != defines_node.end(); iter++)
        {
            std::string key = iter->first.as<std::string>();

            if (!iter->second.IsScalar())
            {
                db_error(asset, "[%s] define '%s' for technique '%s' was not scalar type.", path, key.c_str(), name);
                return false;
            }

            std::string value = iter->second.as<std::string>();

            block.defines.insert_or_assign(key, value);
        }
    }

    return true;
}

bool shader_loader::parse_effects(const char* path, YAML::Node& node, shader& asset)
{
    YAML::Node effects_node = node["effects"];

    if (!effects_node.IsDefined())
    {
        return true;
    }

    if (effects_node.Type() != YAML::NodeType::Map)
    {
        db_error(asset, "[%s] effects node is invalid data type.", path);
        return false;
    }

    for (auto iter = effects_node.begin(); iter != effects_node.end(); iter++)
    {
        std::string name = iter->first.as<std::string>();

        if (!iter->second.IsMap())
        {
            db_error(asset, "[%s] effectnode '%s' was not map type.", path, name.c_str());
            return false;
        }

        YAML::Node child = iter->second;
        if (!parse_effect(path, name.c_str(), child, asset))
        {
            return false;
        }
    }

    return true;
}

bool shader_loader::parse_effect(const char* path, const char* name, YAML::Node& node, shader& asset)
{
    shader::effect& block = asset.effects.emplace_back();
    block.name = name;

    YAML::Node techniques_node = node["techniques"];

    if (!techniques_node.IsDefined())
    {
        db_error(asset, "[%s] techniques not defined for effect '%s'.", path, name);
        return false;
    }
    if (techniques_node.Type() != YAML::NodeType::Map)
    {
        db_error(asset, "[%s] techniques for effect '%s' was not a map type.", path, name);
        return false;
    }

    for (auto iter = techniques_node.begin(); iter != techniques_node.end(); iter++)
    {
        shader::effect::technique& field = block.techniques.emplace_back();
        field.name = iter->first.as<std::string>();

        if (!iter->second.IsMap())
        {
            db_error(asset, "[%s] effect technique node '%s' was not map type.", path, field.name.c_str());
            return false;
        }

        // Read all variations.
        for (auto var_iter = iter->second.begin(); var_iter != iter->second.end(); var_iter++)
        {
            std::string name = var_iter->first.as<std::string>();

            if (!var_iter->second.IsSequence())
            {
                db_error(asset, "[%s] effect variation node '%s' was not sequence type.", path, name.c_str());
                return false;
            }

            YAML::Node child = var_iter->second;
            if (!parse_variation(path, name.c_str(), child, field.variations))
            {
                return false;
            }
        }
    }

    return true;
}

bool shader_loader::parse_file(const char* path, shader& asset)
{
    db_verbose(asset, "[%s] Parsing file", path);

    YAML::Node node;
    if (!load_asset_descriptor(path, node, k_asset_descriptor_type, k_asset_descriptor_minimum_version, k_asset_descriptor_current_version))
    {
        return false;
    }

    if (!parse_imports(path, node, asset))
    {
        return false;
    }

    if (!parse_param_blocks(path, node, asset))
    {
        return false;
    }

    if (!parse_render_states(path, node, asset))
    {
        return false;
    }

    if (!parse_variations(path, node, asset))
    {
        return false;
    }

    if (!parse_vertex_layouts(path, node, asset))
    {
        return false;
    }

    if (!parse_techniques(path, node, asset))
    {
        return false;
    }

    if (!parse_effects(path, node, asset))
    {
        return false;
    }

    return true;
}

std::string shader_loader::create_autogenerated_stub(shader::technique& technique, shader& asset)
{
    std::string result = "";
    size_t cbuffer_register_count = 0;

    // Add bindless arrays.
    result += "Texture1D table_texture_1d[] : register(t0, space1);\n";
    result += "Texture2D table_texture_2d[] : register(t0, space2);\n";
    result += "Texture3D table_texture_3d[] : register(t0, space3);\n";
    result += "TextureCube table_texture_cube[] : register(t0, space4);\n";
    result += "sampler table_samplers[] : register(t0, space5);\n";
    result += "ByteAddressBuffer table_vertex_data[] : register(t0, space6);\n";
    result += "\n";

    // Add param block struct definitions.
    for (int index : technique.param_block_indices)
    {
        shader::param_block& block = asset.param_blocks[index];

        // Create the cbuffer.
        result += string_format("cbuffer %s : register(b%i) {\n", block.name.c_str(), cbuffer_register_count++);
        for (shader::param_block::field& field : block.fields)
        {
            std::string data_type = render_data_type_hlsl_type[static_cast<int>(field.data_type)];
            std::string field_name = field.name;

            // Textures / samplers are accessed bindlessly, so swap them to uint 
            // indices into our tables.
            if (field.data_type == render_data_type::t_texture1d ||
                field.data_type == render_data_type::t_texture2d ||
                field.data_type == render_data_type::t_texture3d ||
                field.data_type == render_data_type::t_texturecube ||
                field.data_type == render_data_type::t_sampler)
            {
                data_type = "uint";
                field_name += "_index";
            }

            result += string_format("\t%s %s;\n", data_type.c_str(), field_name.c_str());
        }
        result += "};\n";
        result += "\n";

        // Add defines for accessing textures.
        size_t texture_defines = 0;

        for (shader::param_block::field& field : block.fields)
        {
            std::string table_name = "";

            if (field.data_type == render_data_type::t_texture1d)
            {
                table_name = "table_texture_1d";
            }
            else if (field.data_type == render_data_type::t_texture2d)
            {
                table_name = "table_texture_2d";
            }
            else if (field.data_type == render_data_type::t_texture3d)
            {
                table_name = "table_texture_3d";
            }
            else if (field.data_type == render_data_type::t_texturecube)
            {
                table_name = "table_texture_cube";
            }
            else if (field.data_type == render_data_type::t_sampler)
            {
                table_name = "table_samplers";
            }
            else
            {
                continue;
            }

            result += string_format("#define %s %s[%s_index]\n",
                field.name.c_str(),
                table_name.c_str(), 
                field.name.c_str()
            );
            texture_defines++;
        }

        if (texture_defines > 0)
        {
            result += "\n";
        }
    }

    // Add vertex layout struct.
    shader::vertex_layout& layout = asset.vertex_layouts[technique.vertex_layout_index];
    result += string_format("struct vertex {\n");
    for (shader::vertex_layout::field& field : layout.fields)
    {
        std::string data_type = render_data_type_hlsl_type[static_cast<int>(field.data_type)];
        std::string field_name = field.name;

        result += string_format("\t%s %s;\n", data_type.c_str(), field_name.c_str());
    }
    result += "};\n";
    result += "\n";

    // Add cbuffer for vertex buffer data + define for loading vertices.
    result += string_format("cbuffer vertex_info : register(b%i) {\n", cbuffer_register_count++);
    result += "\tuint vertex_buffer_index;\n";
    result += "\tuint vertex_buffer_offset;\n";
    result += "};\n";
    result += "\n";
    result += "#define load_vertex(vertex_id) table_vertex_data[vertex_buffer_index].Load<vertex>((vertex_buffer_offset + vertex_id) * sizeof(vertex))\n";
    result += "\n";

    return result;
}

bool shader_loader::compile_technique(const char* path, shader::technique& technique, shader& asset, platform_type asset_platform, config_type asset_config)
{
    std::unique_ptr<render_shader_compiler> compiler = m_engine.get_render_interface().create_shader_compiler();

    for (size_t i = 0; i < technique.stages.size(); i++)
    {
        shader::technique::stage& stage = technique.stages[i];
        render_shader_stage pipeline_stage = static_cast<render_shader_stage>(i);

        if (stage.file.empty())
        {
            continue;
        }

        std::string source_code = "";

        // Read in all text from shader source.
        {
            std::unique_ptr<stream> stream = virtual_file_system::get().open(stage.file.c_str(), false);
            if (!stream)
            {
                db_error(asset, "[%s] Failed to open stream to shader source.", path);
                return false;
            }

            source_code = stream->read_all_string();
        }

        // Prefix the file with autogenerated stub code for param block structs etc.
        std::string stub = create_autogenerated_stub(technique, asset);
        source_code = stub + "\n" + source_code;

        // Add some system defines.
        std::unordered_map<std::string, std::string> defines = technique.defines;
        if (asset_config == config_type::debug)
        {
            defines["WS_DEBUG"] = "1";
        }
        else
        {
            defines["WS_RELEASE"] = "1";
        }

        // Remember this file as a compile dependency.
        asset.header.add_dependency(stage.file.c_str());

        // Compiler source.
        render_shader_compiler_output output = compiler->compile(
            pipeline_stage, 
            source_code.c_str(),
            stage.file.c_str(), 
            stage.entry_point.c_str(),
            defines,
            (asset_config == config_type::debug)
        );

        if (output.success())
        {
            stage.bytecode = output.get_bytecode();

            for (const std::string& dependency : output.get_dependencies())
            {
                asset.header.add_dependency(dependency.c_str());
            }
        }
        else
        {
            return false;
        }
    }

    return true;
}

bool shader_loader::compile(const char* input_path, const char* output_path, platform_type asset_platform, config_type asset_config)
{
    shader asset;
    
    // Parse the source YAML file that defines the shader.
    if (!parse_file(input_path, asset))
    {
        return false;
    }

    // For each technique, compile the shader bytecode.
    for (shader::technique& technique : asset.techniques)
    {
        db_log(asset, "[%s] compiling shader technique '%s'.", input_path, technique.name.c_str());

        if (!compile_technique(input_path, technique, asset, asset_platform, asset_config))
        {
            return false;
        }
    }

    // Construct the asset header.
    asset_cache_key compiled_key;
    if (!get_cache_key(input_path, asset_platform, asset_config, compiled_key, asset.header.dependencies))
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

size_t shader_loader::get_compiled_version()
{
    return k_asset_compiled_version;
}

}; // namespace ws

