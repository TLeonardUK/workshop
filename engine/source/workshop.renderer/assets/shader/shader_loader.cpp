// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/assets/shader/shader_loader.h"
#include "workshop.renderer/assets/shader/shader.h"
#include "workshop.renderer/assets/material/material.h"
#include "workshop.assets/asset_cache.h"
#include "workshop.core/filesystem/file.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/filesystem/virtual_file_system.h"

#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_shader_compiler.h"

#include "thirdparty/yamlcpp/include/yaml-cpp/yaml.h"

namespace ws {

namespace {

constexpr const char* k_asset_descriptor_type = "shader";
constexpr size_t k_asset_descriptor_minimum_version = 1;
constexpr size_t k_asset_descriptor_current_version = 1;

// Bump if compiled format ever changes.
constexpr size_t k_asset_compiled_version = 24;

};

template<>
inline void stream_serialize(stream& out, shader::param_block& block)
{
    stream_serialize(out, block.name);
    stream_serialize_enum(out, block.scope);

    stream_serialize_list(out, block.layout.fields, [&out](ri_data_layout::field& field) {
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

    stream_serialize_list(out, block.layout.fields, [&out](ri_data_layout::field& field) {
        stream_serialize(out, field.name);
        stream_serialize_enum(out, field.data_type);
    });
}

template<>
inline void stream_serialize(stream& out, shader::output_target& block)
{
    stream_serialize(out, block.name);

    stream_serialize_list(out, block.color, [&out](ri_texture_format& format) {
        stream_serialize_enum(out, format);
    });

    stream_serialize_enum(out, block.depth);
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
inline void stream_serialize(stream& out, shader::ray_hitgroup& block)
{
    stream_serialize(out, block.name);
    stream_serialize_enum(out, block.domain);
    stream_serialize_enum(out, block.type);

    for (size_t i = 0; i < block.stages.size(); i++)
    {
        shader::shader_stage& stage = block.stages[i];
        stream_serialize(out, stage.file);
        stream_serialize(out, stage.entry_point);
        stream_serialize_list(out, stage.bytecode);
    }
}

template<>
inline void stream_serialize(stream& out, shader::ray_missgroup& block)
{
    stream_serialize(out, block.name);
    stream_serialize_enum(out, block.type);

    shader::shader_stage& stage = block.ray_miss_stage;
    stream_serialize(out, stage.file);
    stream_serialize(out, stage.entry_point);
    stream_serialize_list(out, stage.bytecode);
}

template<>
inline void stream_serialize(stream& out, shader::technique& block)
{
    stream_serialize(out, block.name);

    for (size_t i = 0; i < block.stages.size(); i++)
    {
        shader::shader_stage& stage = block.stages[i];
        stream_serialize(out, stage.file);
        stream_serialize(out, stage.entry_point);
        stream_serialize_list(out, stage.bytecode);
    }

    stream_serialize(out, block.render_state_index);
    stream_serialize(out, block.vertex_layout_index);
    stream_serialize(out, block.output_target_index);
    stream_serialize_list(out, block.param_block_indices);
    stream_serialize_list(out, block.ray_hitgroups);
    stream_serialize_list(out, block.ray_missgroups);
    stream_serialize_map(out, block.defines);
}

shader_loader::shader_loader(ri_interface& instance, renderer& renderer)
    : m_ri_interface(instance)
    , m_renderer(renderer)
{
}

const std::type_info& shader_loader::get_type()
{
    return typeid(shader);
}

const char* shader_loader::get_descriptor_type()
{
    return k_asset_descriptor_type;
}

asset* shader_loader::get_default_asset()
{
    return nullptr;
}

asset* shader_loader::load(const char* path)
{
    shader* asset = new shader(m_ri_interface, m_renderer);
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
        asset.name = path;
    }

    if (!serialize_header(*stream, asset.header, path))
    {
        return false;
    }
        
    stream_serialize_list(*stream, asset.param_blocks);
    stream_serialize_list(*stream, asset.render_states);
    stream_serialize_list(*stream, asset.variations);
    stream_serialize_list(*stream, asset.vertex_layouts);
    stream_serialize_list(*stream, asset.output_targets);
    stream_serialize_list(*stream, asset.effects);
    stream_serialize_list(*stream, asset.techniques);
    // stream_serialize_list(*stream, asset.ray_hitgroups); // technique instances these, no need to keep the global list around.
    // stream_serialize_list(*stream, asset.ray_missgroups); // technique instances these, no need to keep the global list around.
    stream_serialize_map(*stream, asset.global_defines);

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

bool shader_loader::parse_defines(const char* path, YAML::Node& node, shader& asset)
{
    YAML::Node defines_node = node["defines"];

    if (!defines_node.IsDefined())
    {
        return true;
    }

    if (defines_node.Type() != YAML::NodeType::Map)
    {
        db_error(asset, "[%s] global defines blocks was not a map type.", path);
        return false;
    }

    for (auto iter = defines_node.begin(); iter != defines_node.end(); iter++)
    {
        std::string key = iter->first.as<std::string>();

        if (!iter->second.IsScalar())
        {
            db_error(asset, "[%s] global define '%s' was not scalar type.", path, key.c_str());
            return false;
        }

        std::string value = iter->second.as<std::string>();

        asset.global_defines.insert_or_assign(key, value);
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

    if (auto ret = from_string<ri_data_scope>(scope_node.as<std::string>()); ret)
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
        ri_data_layout::field& field = block.layout.fields.emplace_back();
        field.name = iter->first.as<std::string>();

        if (!iter->second.IsScalar())
        {
            db_error(asset, "[%s] param block field '%s' was not map type.", path, field.name.c_str());
            return false;
        }

        std::string field_data_type = iter->second.as<std::string>();

        if (auto ret = from_string<ri_data_type>(field_data_type.c_str()); ret)
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

bool shader_loader::parse_ray_hitgroups(const char* path, YAML::Node& node, shader& asset)
{
    YAML::Node ray_hitgroups_node = node["ray_hitgroups"];

    if (!ray_hitgroups_node.IsDefined())
    {
        return true;
    }

    if (ray_hitgroups_node.Type() != YAML::NodeType::Map)
    {
        db_error(asset, "[%s] ray_hitgroups node is invalid data type.", path);
        return false;
    }

    for (auto iter = ray_hitgroups_node.begin(); iter != ray_hitgroups_node.end(); iter++)
    {
        std::string name = iter->first.as<std::string>();

        if (!iter->second.IsMap())
        {
            db_error(asset, "[%s] ray hitgroups node '%s' was not map type.", path, name.c_str());
            return false;
        }

        YAML::Node child = iter->second;
        if (!parse_ray_hitgroup(path, name.c_str(), child, asset))
        {
            return false;
        }
    }

    return true;
}

bool shader_loader::parse_ray_hitgroup(const char* path, const char* name, YAML::Node& node, shader& asset)
{
    YAML::Node material_domain_group = node["material_domain"];
    YAML::Node ray_type_group = node["ray_type"];

    shader::ray_hitgroup& block = asset.ray_hitgroups.emplace_back();
    block.name = name;

    if (!material_domain_group.IsDefined())
    {
        db_error(asset, "[%s] material_domain not defined for ray hit group '%s'.", path, name);
        return false;
    }
    if (material_domain_group.Type() != YAML::NodeType::Scalar)
    {
        db_error(asset, "[%s] material_domain for ray hit group '%s' was not a scalar type.", path, name);
        return false;
    }

    if (!ray_type_group.IsDefined())
    {
        db_error(asset, "[%s] ray_type not defined for ray hit group '%s'.", path, name);
        return false;
    }
    if (ray_type_group.Type() != YAML::NodeType::Scalar)
    {
        db_error(asset, "[%s] ray_type for ray hit group '%s' was not a scalar type.", path, name);
        return false;
    }

    if (auto ret = from_string<material_domain>(material_domain_group.as<std::string>()); ret)
    {
        block.domain = ret.get();
    }
    else
    {
        db_error(asset, "[%s] material_domain for ray hit group '%s' is invalid type '%s'.", path, material_domain_group.as<std::string>().c_str());
        return false;
    }

    if (auto ret = from_string<ray_type>(ray_type_group.as<std::string>()); ret)
    {
        block.type = ret.get();
    }
    else
    {
        db_error(asset, "[%s] ray_type for ray hit group '%s' is invalid type '%s'.", path, ray_type_group.as<std::string>().c_str());
        return false;
    }

    size_t loaded_stage_count = 0;

    if (!parse_shader_stages(path, name, node, asset, block.stages, loaded_stage_count))
    {
        return false;
    }

    if (loaded_stage_count == 0)
    {
        db_error(asset, "[%s] ray hitgroup '%s' defines no shader stages.", path, name);
        return false;
    }

    return true;
}

bool shader_loader::parse_ray_missgroups(const char* path, YAML::Node& node, shader& asset)
{
    YAML::Node ray_hitgroups_node = node["ray_missgroups"];

    if (!ray_hitgroups_node.IsDefined())
    {
        return true;
    }

    if (ray_hitgroups_node.Type() != YAML::NodeType::Map)
    {
        db_error(asset, "[%s] ray_missgroups node is invalid data type.", path);
        return false;
    }

    for (auto iter = ray_hitgroups_node.begin(); iter != ray_hitgroups_node.end(); iter++)
    {
        std::string name = iter->first.as<std::string>();

        if (!iter->second.IsMap())
        {
            db_error(asset, "[%s] ray_missgroups node '%s' was not map type.", path, name.c_str());
            return false;
        }

        YAML::Node child = iter->second;
        if (!parse_ray_missgroup(path, name.c_str(), child, asset))
        {
            return false;
        }
    }

    return true;
}

bool shader_loader::parse_ray_missgroup(const char* path, const char* name, YAML::Node& node, shader& asset)
{
    YAML::Node ray_type_group = node["ray_type"];

    shader::ray_missgroup& block = asset.ray_missgroups.emplace_back();
    block.name = name;

    if (!ray_type_group.IsDefined())
    {
        db_error(asset, "[%s] ray_type not defined for ray miss group '%s'.", path, name);
        return false;
    }
    if (ray_type_group.Type() != YAML::NodeType::Scalar)
    {
        db_error(asset, "[%s] ray_type for ray miss group '%s' was not a scalar type.", path, name);
        return false;
    }

    if (auto ret = from_string<ray_type>(ray_type_group.as<std::string>()); ret)
    {
        block.type = ret.get();
    }
    else
    {
        db_error(asset, "[%s] ray_type for ray miss group '%s' is invalid type '%s'.", path, ray_type_group.as<std::string>().c_str());
        return false;
    }

    size_t loaded_stage_count = 0;
    std::array<shader::shader_stage, static_cast<int>(ri_shader_stage::COUNT)> stages;

    if (!parse_shader_stages(path, name, node, asset, stages, loaded_stage_count))
    {
        return false;
    }

    if (loaded_stage_count == 0 || stages[(int)ri_shader_stage::ray_miss].entry_point.empty())
    {
        db_error(asset, "[%s] ray missgroup '%s' defines no miss shader stage.", path, name);
        return false;
    }

    block.ray_miss_stage = stages[(int)ri_shader_stage::ray_miss];

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

    auto ReadVariable = [&path, &node, &variables_valid](const std::string& name, auto& source, auto default_value)
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

    ReadVariable("topology",                        block.state.topology,                           ri_topology::triangle);
    ReadVariable("fill_mode",                       block.state.fill_mode,                          ri_fill_mode::solid);
    ReadVariable("cull_mode",                       block.state.cull_mode,                          ri_cull_mode::back);
    ReadVariable("depth_bias",                      block.state.depth_bias,                         0u);
    ReadVariable("depth_bias_clamp",                block.state.depth_bias_clamp,                   0.0f);
    ReadVariable("slope_scaled_depth_bias",         block.state.slope_scaled_depth_bias,            0.0f);
    ReadVariable("depth_clip_enabled",              block.state.depth_clip_enabled,                 true);
    ReadVariable("multisample_enabled",             block.state.multisample_enabled,                false);
    ReadVariable("multisample_count",               block.state.multisample_count,                  1);
    ReadVariable("antialiased_line_enabled",        block.state.antialiased_line_enabled,           false);
    ReadVariable("conservative_raster_enabled",     block.state.conservative_raster_enabled,        false);

    ReadVariable("alpha_to_coverage",               block.state.alpha_to_coverage,                  false);
    ReadVariable("independent_blend_enabled",       block.state.independent_blend_enabled,          false);    

    ReadVariable("max_rt_payload_size",             block.state.max_rt_payload_size,                32);


    for (size_t i = 0; i < ri_pipeline_render_state::k_max_output_targets; i++)
    {
        ReadVariable("blend"+std::to_string(i)+"_enabled",                   block.state.blend_enabled[i],                      false);
        ReadVariable("blend"+std::to_string(i)+"_op",                        block.state.blend_op[i],                           ri_blend_op::add);
        ReadVariable("blend"+std::to_string(i)+"_source_op",                 block.state.blend_source_op[i],                    ri_blend_operand::one);
        ReadVariable("blend"+std::to_string(i)+"_destination_op",            block.state.blend_destination_op[i],               ri_blend_operand::zero);
        ReadVariable("blend"+std::to_string(i)+"_alpha_op",                  block.state.blend_alpha_op[i],                     ri_blend_op::add);
        ReadVariable("blend"+std::to_string(i)+"_alpha_source_op",           block.state.blend_alpha_source_op[i],              ri_blend_operand::one);
        ReadVariable("blend"+std::to_string(i)+"_alpha_destination_op",      block.state.blend_alpha_destination_op[i],         ri_blend_operand::zero);
    }

    ReadVariable("depth_test_enabled",              block.state.depth_test_enabled,                 true);
    ReadVariable("depth_write_enabled",             block.state.depth_write_enabled,                true);
    ReadVariable("depth_compare_op",                block.state.depth_compare_op,                   ri_compare_op::less);

    ReadVariable("stencil_test_enabled",            block.state.stencil_test_enabled,               false);
    ReadVariable("stencil_read_mask",               block.state.stencil_read_mask,                  0u);
    ReadVariable("stencil_write_mask",              block.state.stencil_write_mask,                 0u);
    ReadVariable("stencil_front_face_fail_op",      block.state.stencil_front_face_fail_op,         ri_stencil_op::keep);
    ReadVariable("stencil_front_face_depth_fail_op",block.state.stencil_front_face_depth_fail_op,   ri_stencil_op::keep);
    ReadVariable("stencil_front_face_pass_op",      block.state.stencil_front_face_pass_op,         ri_stencil_op::keep);
    ReadVariable("stencil_front_face_compare_op",   block.state.stencil_front_face_compare_op,      ri_compare_op::always);
    ReadVariable("stencil_back_face_fail_op",       block.state.stencil_back_face_fail_op,          ri_stencil_op::keep);
    ReadVariable("stencil_back_face_depth_fail_op", block.state.stencil_back_face_depth_fail_op,    ri_stencil_op::keep);
    ReadVariable("stencil_back_face_pass_op",       block.state.stencil_back_face_pass_op,          ri_stencil_op::keep);
    ReadVariable("stencil_back_face_compare_op",    block.state.stencil_back_face_compare_op,       ri_compare_op::always);

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

        if (!iter->second.IsSequence())
        {
            db_error(asset, "[%s] vertex layout node '%s' was not sequence type.", path, name.c_str());
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
        ri_data_layout::field& field = block.layout.fields.emplace_back();
        field.name = iter->as<std::string>();
        field.data_type = ri_data_type::t_float4; // data type is not relevant, the types are chosen internally by the engine and unpacked in the shader.
    }

    return true;
}

bool shader_loader::parse_output_targets(const char* path, YAML::Node& node, shader& asset)
{
    YAML::Node output_targets_node = node["output_targets"];

    if (!output_targets_node.IsDefined())
    {
        return true;
    }

    if (output_targets_node.Type() != YAML::NodeType::Map)
    {
        db_error(asset, "[%s] output_targets node is invalid data type.", path);
        return false;
    }

    for (auto iter = output_targets_node.begin(); iter != output_targets_node.end(); iter++)
    {
        std::string name = iter->first.as<std::string>();

        if (!iter->second.IsMap())
        {
            db_error(asset, "[%s] output target node '%s' was not map type.", path, name.c_str());
            return false;
        }

        YAML::Node child = iter->second;
        if (!parse_output_target(path, name.c_str(), child, asset))
        {
            return false;
        }
    }

    return true;
}

bool shader_loader::parse_output_target(const char* path, const char* name, YAML::Node& node, shader& asset)
{
    shader::output_target& block = asset.output_targets.emplace_back();
    block.name = name;
    block.depth = ri_texture_format::Undefined;

    YAML::Node color_node = node["color"];
    YAML::Node depth_node = node["depth"];

    if (color_node.IsDefined())
    {
        for (auto iter = color_node.begin(); iter != color_node.end(); iter++)
        {
            if (!iter->IsScalar())
            {
                db_error(asset, "[%s] color value for '%s' was not scalar type.", path, name);
                return false;
            }

            std::string format_string = iter->as<std::string>();
            result<ri_texture_format> format = from_string<ri_texture_format>(format_string);
            if (!format)
            {
                db_error(asset, "[%s] color value for '%s' has unknown type '%s'.", path, name, format_string.c_str());
                return false;
            }

            block.color.push_back(format.get());
        }
    }

    if (depth_node.IsDefined())
    {
        if (!depth_node.IsScalar())
        {
            db_error(asset, "[%s] depth value for '%s' was not scalar type.", path, name);
            return false;
        }

        std::string format_string = depth_node.as<std::string>();
        result<ri_texture_format> format = from_string<ri_texture_format>(format_string);
        if (!format)
        {
            db_error(asset, "[%s] depth value for '%s' has unknown type '%s'.", path, name, format_string.c_str());
            return false;
        }

        block.depth = format.get();
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

bool shader_loader::parse_shader_stages(const char* path, const char* name, YAML::Node& node, shader& asset, std::array<shader::shader_stage, static_cast<int>(ri_shader_stage::COUNT)>& stages, size_t& loaded_stage_count)
{
    std::array<YAML::Node, static_cast<int>(ri_shader_stage::COUNT)> stage_nodes = {
        node["vertex_shader"],
        node["pixel_shader"],
        node["domain_shader"],
        node["hull_shader"],
        node["geometry_shader"],
        node["compute_shader"],

        node["ray_generation_shader"],
        node["ray_any_hit_shader"],
        node["ray_closest_hit_shader"],
        node["ray_miss_shader"],
        node["ray_intersection_shader"],
    };

    // Parse shader stages.
    for (size_t i = 0; i < stage_nodes.size(); i++)
    {
        ri_shader_stage stage = static_cast<ri_shader_stage>(i);
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

        stages[i].entry_point = entry_node.as<std::string>();
        stages[i].file = file_node.as<std::string>();
        loaded_stage_count++;
    }

    return true;
}

bool shader_loader::parse_technique(const char* path, const char* name, YAML::Node& node, shader& asset)
{
    YAML::Node render_state_node = node["render_state"];
    YAML::Node vertex_layout_node = node["vertex_layout"];
    YAML::Node output_target_node = node["output_target"];
    YAML::Node param_blocks_node = node["param_blocks"];
    YAML::Node defines_node = node["defines"];
    YAML::Node ray_hitgroups_node = node["ray_hitgroups"];
    YAML::Node ray_missgroups_node = node["ray_missgroups"];

    shader::technique& block = asset.techniques.emplace_back();
    block.name = name;

    size_t loaded_stage_count = 0;

    if (!parse_shader_stages(path, name, node, asset, block.stages, loaded_stage_count))
    {
        return false;
    }

    if (loaded_stage_count == 0)
    {
        db_error(asset, "[%s] technique '%s' defines no shader stages.", path, name);
        return false;
    }

    bool is_compute = !block.stages[(int)ri_shader_stage::compute].file.empty() ||
                      !block.stages[(int)ri_shader_stage::ray_any_hit].file.empty() ||
                      !block.stages[(int)ri_shader_stage::ray_closest_hit].file.empty() ||
                      !block.stages[(int)ri_shader_stage::ray_generation].file.empty() ||
                      !block.stages[(int)ri_shader_stage::ray_intersection].file.empty() ||
                      !block.stages[(int)ri_shader_stage::ray_miss].file.empty();

    if (!is_compute)
    {
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

        // Parse output target
        if (!output_target_node.IsDefined())
        {
            db_error(asset, "[%s] technique '%s' has no defined output target.", path, name);
            return false;
        }

        if (output_target_node.Type() != YAML::NodeType::Scalar)
        {
            db_error(asset, "[%s] output layout for technique '%s' was not a scalar type.", path, name);
            return false;
        }
    
        std::string output_target_name = output_target_node.as<std::string>();
        auto output_target_iter = std::find_if(asset.output_targets.begin(), asset.output_targets.end(), [&output_target_name](const shader::output_target& state) {
            return state.name == output_target_name;
        });
        if (output_target_iter == asset.output_targets.end())
        {
            db_error(asset, "[%s] output target '%s' for technique '%s' was not found.", path, output_target_name.c_str(), name);
            return false;                
        }
        else
        {
            block.output_target_index = std::distance(asset.output_targets.begin(), output_target_iter);
        }
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

    // Parse ray hitgroups
    if (ray_hitgroups_node.IsDefined())
    {
        if (ray_hitgroups_node.Type() != YAML::NodeType::Sequence)
        {
            db_error(asset, "[%s] ray hit groups for technique '%s' was not a sequence type.", path, name);
            return false;
        }

        for (auto iter = ray_hitgroups_node.begin(); iter != ray_hitgroups_node.end(); iter++)
        {
            if (!iter->IsScalar())
            {
                db_error(asset, "[%s] ray hit group value for technique '%s' was not scalar type.", path, name);
                return false;
            }

            std::string group_name = iter->as<std::string>();
            auto group_iter = std::find_if(asset.ray_hitgroups.begin(), asset.ray_hitgroups.end(), [&group_name](const shader::ray_hitgroup& state) {
                return state.name == group_name;
            });
            if (group_iter == asset.ray_hitgroups.end())
            {
                db_error(asset, "[%s] ray hitgroups '%s' for technique '%s' was not found.", path, group_name.c_str(), name);
                return false;
            }
            else
            {
                block.ray_hitgroups.push_back(*group_iter);
            }
        }
    }

    // Parse ray missgroups
    if (ray_missgroups_node.IsDefined())
    {
        if (ray_missgroups_node.Type() != YAML::NodeType::Sequence)
        {
            db_error(asset, "[%s] ray miss groups for technique '%s' was not a sequence type.", path, name);
            return false;
        }

        for (auto iter = ray_missgroups_node.begin(); iter != ray_missgroups_node.end(); iter++)
        {
            if (!iter->IsScalar())
            {
                db_error(asset, "[%s] ray miss group value for technique '%s' was not scalar type.", path, name);
                return false;
            }

            std::string group_name = iter->as<std::string>();
            auto group_iter = std::find_if(asset.ray_missgroups.begin(), asset.ray_missgroups.end(), [&group_name](const shader::ray_missgroup& state) {
                return state.name == group_name;
                });
            if (group_iter == asset.ray_missgroups.end())
            {
                db_error(asset, "[%s] ray missgroups '%s' for technique '%s' was not found.", path, group_name.c_str(), name);
                return false;
            }
            else
            {
                block.ray_missgroups.push_back(*group_iter);
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

    if (!parse_defines(path, node, asset))
    {
        return false;
    }

    if (!parse_param_blocks(path, node, asset))
    {
        return false;
    }

    if (!parse_ray_hitgroups(path, node, asset))
    {
        return false;
    }

    if (!parse_ray_missgroups(path, node, asset))
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

    if (!parse_output_targets(path, node, asset))
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

#pragma optimize("", off)

std::string shader_loader::create_autogenerated_stub(shader::technique& technique, shader& asset)
{
    std::string result = "";
    size_t cbuffer_register_count = 0;


    bool is_compute = !technique.stages[(int)ri_shader_stage::compute].file.empty() ||
                      !technique.stages[(int)ri_shader_stage::ray_any_hit].file.empty() ||
                      !technique.stages[(int)ri_shader_stage::ray_closest_hit].file.empty() ||
                      !technique.stages[(int)ri_shader_stage::ray_generation].file.empty() ||
                      !technique.stages[(int)ri_shader_stage::ray_intersection].file.empty() ||
                      !technique.stages[(int)ri_shader_stage::ray_miss].file.empty();

    result += "// ================================================================================================\n";
    result += "//  workshop\n";
    result += "//  Copyright (C) 2023 Tim Leonard\n";
    result += "//\n";
    result += "//  This header has been autogenerated by the shader loader.\n";
    result += "// ================================================================================================\n";
    result += "\n";

    // Add bindless arrays.
    // Note: If you change these, make sure you update the code
    //       in shader::post_load to create the pipeline correctly.
    result += "Texture1D table_texture_1d[] : register(t0, space1);\n";
    result += "Texture2D table_texture_2d[] : register(t0, space2);\n";
    result += "Texture3D table_texture_3d[] : register(t0, space3);\n";
    result += "TextureCube table_texture_cube[] : register(t0, space4);\n";
    result += "sampler table_samplers[] : register(t0, space5);\n";
    result += "ByteAddressBuffer table_byte_buffers[] : register(t0, space6);\n";
    result += "RWByteAddressBuffer table_rw_byte_buffers[] : register(u0, space7);\n";
    result += "RWTexture2D<float4> table_rw_texture_2d[] : register(u0, space8);\n";
    result += "RaytracingAccelerationStructure table_tlas[] : register(t0, space9);\n";
    result += "\n";

    // Add param block struct definitions.
    for (uint64_t index : technique.param_block_indices)
    {
        shader::param_block& block = asset.param_blocks[index];

        // Create the cbuffer.
        if (block.scope == ri_data_scope::instance || block.scope == ri_data_scope::indirect)
        {
            result += string_format("struct %s {\n", block.name.c_str());
        }
        else
        {
            result += string_format("cbuffer %s : register(b%i) {\n", block.name.c_str(), cbuffer_register_count++);
        }

        for (ri_data_layout::field& field : block.layout.fields)
        {
            std::string data_type = ri_data_type_hlsl_type[static_cast<int>(field.data_type)];
            std::string field_name = field.name;

            // Textures / samplers are accessed bindlessly, so swap them to uint 
            // indices into our tables.
            if (field.data_type == ri_data_type::t_texture1d ||
                field.data_type == ri_data_type::t_texture2d ||
                field.data_type == ri_data_type::t_texture3d ||
                field.data_type == ri_data_type::t_texturecube ||
                field.data_type == ri_data_type::t_sampler ||
                field.data_type == ri_data_type::t_byteaddressbuffer ||
                field.data_type == ri_data_type::t_rwbyteaddressbuffer ||
                field.data_type == ri_data_type::t_rwtexture2d ||
                field.data_type == ri_data_type::t_tlas)
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

        if (block.scope != ri_data_scope::instance)
        {
            for (ri_data_layout::field& field : block.layout.fields)
            {
                std::string table_name = "";

                if (field.data_type == ri_data_type::t_texture1d)
                {
                    table_name = "table_texture_1d";
                }
                else if (field.data_type == ri_data_type::t_texture2d)
                {
                    table_name = "table_texture_2d";
                }
                else if (field.data_type == ri_data_type::t_texture3d)
                {
                    table_name = "table_texture_3d";
                }
                else if (field.data_type == ri_data_type::t_texturecube)
                {
                    table_name = "table_texture_cube";
                }
                else if (field.data_type == ri_data_type::t_sampler)
                {
                    table_name = "table_samplers";
                }
                else if (field.data_type == ri_data_type::t_byteaddressbuffer)
                {
                    table_name = "table_byte_buffers";
                }
                else if (field.data_type == ri_data_type::t_rwbyteaddressbuffer)
                {
                    table_name = "table_rw_byte_buffers";
                }
                else if (field.data_type == ri_data_type::t_rwtexture2d)
                {
                    table_name = "table_rw_texture_2d";
                }
                else if (field.data_type == ri_data_type::t_tlas)
                {
                    table_name = "table_tlas";
                }
                else
                {
                    continue;
                }

                if (block.scope != ri_data_scope::indirect)
                {
                    result += string_format("#define %s %s[%s_index]\n",
                        field.name.c_str(),
                        table_name.c_str(), 
                        field.name.c_str()
                    );
                }
                texture_defines++;
            }
        }

        if (texture_defines > 0)
        {
            result += "\n";
        }
    }

    result += "\n";
    result += "#include \"data:shaders/source/common/compression.hlsl\"\n";
    result += "#include \"data:shaders/source/common/global.hlsl\"\n";
    result += "\n";

    if (!is_compute)
    {
        shader::vertex_layout& layout = asset.vertex_layouts[technique.vertex_layout_index];

        // Add a function for unpacking a vertex.
        result += "vertex load_vertex(uint vertex_id) {\n";
        result += " model_info info = table_byte_buffers[model_info_table].Load<model_info>(model_info_offset);\n";
        result += " return load_model_vertex(info, vertex_id);\n";
        result += "};\n";
        result += "\n";

        // Add a function for loading material info.
        result += "material load_material() {\n";
        result += " return load_material_from_table(material_info_table, material_info_offset);";
        result += "};\n";
        result += "\n";
    }

    // Add defines for loading each instance param block.
    size_t read_offset = 0;
    size_t total_instance_pbs = 0;

    for (uint64_t index : technique.param_block_indices)
    {
        shader::param_block& block = asset.param_blocks[index];

        if (block.scope == ri_data_scope::instance)
        {
            total_instance_pbs++;
        }
    }

    // Ensure this is kept in sync with the value in common_types.h
    result += "struct instance_offset_info {\n";
    result += "   uint data_buffer_index;\n";
    result += "   uint data_buffer_offset;\n";
    result += "};\n";
    result += "\n";

    if (total_instance_pbs > 0 && !is_compute)
    {
        for (uint64_t index : technique.param_block_indices)
        {
            shader::param_block& block = asset.param_blocks[index];

            if (block.scope == ri_data_scope::instance)
            {
                result += string_format("%s load_%s(uint instance_id)\n", block.name.c_str(), block.name.c_str());
                result += string_format("{\n");
                result += string_format("   instance_offset_info info = instance_buffer.Load<instance_offset_info>((instance_id * %u * sizeof(instance_offset_info)) + (%u * sizeof(instance_offset_info)));\n", total_instance_pbs, read_offset);
                result += string_format("   %s pb = table_byte_buffers[NonUniformResourceIndex(info.data_buffer_index)].Load<%s>(info.data_buffer_offset);\n", block.name.c_str(), block.name.c_str());
                result += string_format("   return pb;\n");
                result += string_format("}\n");
                result += "\n";

                read_offset++;
            }
        }
    }

    result += "\n";

    return result;
}

bool shader_loader::compile_shader_stage(const char* path, shader::technique& technique, shader& asset, platform_type asset_platform, config_type asset_config, shader::shader_stage& stage, ri_shader_stage pipeline_stage)
{
    std::unique_ptr<ri_shader_compiler> compiler = m_ri_interface.create_shader_compiler();

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
    std::string shader_stub = create_autogenerated_stub(technique, asset);
    std::unordered_map<std::string, std::string> defines = technique.defines;

    source_code = shader_stub + "\n" + source_code;

    // Merge in the global defines.
    for (auto& pair : asset.global_defines)
    {
        defines[pair.first] = pair.second;
    }

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
    ri_shader_compiler_output output = compiler->compile(
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
        for (const ri_shader_compiler_output::log& log : output.get_errors())
        {
            db_error(asset, "[%s:%zi] %s", stage.file.c_str(), log.line, log.message.c_str());
            for (const std::string& context : log.context)
            {
                db_error(asset, "[%s:%zi] \t%s", stage.file.c_str(), log.line, context.c_str());
            }
        }
        for (const ri_shader_compiler_output::log& log : output.get_warnings())
        {
            db_warning(asset, "[%s:%zi] %s", stage.file.c_str(), log.line, log.message.c_str());
            for (const std::string& context : log.context)
            {
                db_warning(asset, "[%s:%zi] \t%s", stage.file.c_str(), log.line, context.c_str());
            }
        }
        for (const ri_shader_compiler_output::log& log : output.get_messages())
        {
            db_log(asset, "[%s:%zi] %s", stage.file.c_str(), log.line, log.message.c_str());
            for (const std::string& context : log.context)
            {
                db_log(asset, "[%s:%zi] \t%s", stage.file.c_str(), log.line, context.c_str());
            }
        }

        return false;
    }

    return true;
}

bool shader_loader::compile_technique(const char* path, shader::technique& technique, shader& asset, platform_type asset_platform, config_type asset_config)
{
    for (size_t i = 0; i < technique.stages.size(); i++)
    {
        shader::shader_stage& stage = technique.stages[i];
        ri_shader_stage pipeline_stage = static_cast<ri_shader_stage>(i);

        if (stage.file.empty())
        {
            continue;
        }

        if (!compile_shader_stage(path, technique, asset, asset_platform, asset_config, stage, pipeline_stage))
        {
            return false;
        }
    }

    // For each hitgroup in the technique, compile that.
    for (shader::ray_hitgroup& hitgroup : technique.ray_hitgroups)
    {
        db_log(asset, "[%s] compiling shader hit group '%s' for technique '%s'.", path, hitgroup.name.c_str(), technique.name.c_str());

        if (!compile_ray_hitgroup(path, technique, hitgroup, asset, asset_platform, asset_config))
        {
            return false;
        }
    }

    // For each missgroup in the technique, compile that.
    for (shader::ray_missgroup& missgroup : technique.ray_missgroups)
    {
        db_log(asset, "[%s] compiling shader miss group '%s' for technique '%s'.", path, missgroup.name.c_str(), technique.name.c_str());

        if (!compile_ray_missgroup(path, technique, missgroup, asset, asset_platform, asset_config))
        {
            return false;
        }
    }

    return true;
}

bool shader_loader::compile_ray_hitgroup(const char* path, shader::technique& technique, shader::ray_hitgroup& hitgroup, shader& asset, platform_type asset_platform, config_type asset_config)
{
    for (size_t i = 0; i < hitgroup.stages.size(); i++)
    {
        shader::shader_stage& stage = hitgroup.stages[i];
        ri_shader_stage pipeline_stage = static_cast<ri_shader_stage>(i);

        if (stage.file.empty())
        {
            continue;
        }

        if (!compile_shader_stage(path, technique, asset, asset_platform, asset_config, stage, pipeline_stage))
        {
            return false;
        }
    }

    return true;
}

bool shader_loader::compile_ray_missgroup(const char* path, shader::technique& technique, shader::ray_missgroup& missgroup, shader& asset, platform_type asset_platform, config_type asset_config)
{
    shader::shader_stage& stage = missgroup.ray_miss_stage;
    
    if (stage.file.empty())
    {
        return false;
    }

    if (!compile_shader_stage(path, technique, asset, asset_platform, asset_config, stage, ri_shader_stage::ray_miss))
    {
        return false;
    }
 
    return true;
}

bool shader_loader::compile(const char* input_path, const char* output_path, platform_type asset_platform, config_type asset_config, asset_flags flags)
{
    shader asset(m_ri_interface, m_renderer);
    
    // Parse the source YAML file that defines the shader.
    if (!parse_file(input_path, asset))
    {
        return false;
    }

    // Check if its a compute shader.
    bool is_compute = true;
    for (shader::technique& technique : asset.techniques)
    {
        if (technique.stages[(int)ri_shader_stage::compute].file.empty() &&
            technique.stages[(int)ri_shader_stage::ray_generation].file.empty() &&
            technique.stages[(int)ri_shader_stage::ray_any_hit].file.empty() &&
            technique.stages[(int)ri_shader_stage::ray_closest_hit].file.empty() &&
            technique.stages[(int)ri_shader_stage::ray_intersection].file.empty() &&
            technique.stages[(int)ri_shader_stage::ray_miss].file.empty())
        {
            is_compute = false;
        }
    }

    // Add implicit param blocks
    auto add_implicit_param_block = [&](const char* name)
    {        
        auto model_info_iter = std::find_if(asset.param_blocks.begin(), asset.param_blocks.end(), [&](const shader::param_block& a) { return a.name == name; });
        if (model_info_iter == asset.param_blocks.end())
        {
            db_error(asset, "[%s] Failed to find implicitly required param block '%s'.", input_path, name);
            return false;
        }

        size_t model_info_index = std::distance(asset.param_blocks.begin(), model_info_iter);
        for (shader::technique& technique : asset.techniques)
        {
            technique.param_block_indices.push_back(model_info_index);
        }

        return true;
    };

    if (!is_compute)
    {
       if (!add_implicit_param_block("vertex_info"))
       {
           return false;
       }
    }

    if (!add_implicit_param_block("model_info") ||
        !add_implicit_param_block("material_info"))
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

size_t shader_loader::get_compiled_version()
{
    return k_asset_compiled_version;
}

void shader_loader::hot_reload(asset* instance, asset* new_instance)
{
    shader* old_instance_typed = static_cast<shader*>(instance);
    shader* new_instance_typed = static_cast<shader*>(new_instance);

    old_instance_typed->swap(new_instance_typed);
}

}; // namespace ws

