// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_pipeline.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_buffer.h"
#include "workshop.render_interface.dx12/dx12_types.h"

namespace ws {

dx12_ri_pipeline::dx12_ri_pipeline(dx12_render_interface& renderer, const dx12_ri_pipeline::create_params& params, const char* debug_name)
    : m_renderer(renderer)
    , m_create_params(params)
    , m_debug_name(debug_name)
    , m_is_raytracing(false)
    , m_is_compute(false)
{
}

dx12_ri_pipeline::~dx12_ri_pipeline()
{
    m_renderer.defer_delete([root_signature = m_root_signature, pipeline_state = m_pipeline_state]() 
    {
        //CheckedRelease(root_signature);
        //CheckedRelease(pipeline_state);
    });
}

bool dx12_ri_pipeline::create_root_signature()
{
    // TODO: We should cache these in the dx12_interface, most of our root signatures will be near identical.

    std::vector<D3D12_DESCRIPTOR_RANGE> descriptor_ranges;
    descriptor_ranges.reserve(m_create_params.descriptor_tables.size() + 1);

    std::vector<D3D12_ROOT_PARAMETER> parameters;
    parameters.reserve(m_create_params.descriptor_tables.size() + 1);

    // Add parameters for each bindless array.    
    for (size_t i = 0; i < m_create_params.descriptor_tables.size(); i++)
    {
        ri_descriptor_table& type = m_create_params.descriptor_tables[i];

        D3D12_DESCRIPTOR_RANGE& range = descriptor_ranges.emplace_back();
        range.BaseShaderRegister = 0;
        range.RegisterSpace = static_cast<UINT>(i + 1); // space0 is reserved for non-bindless stuff.
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        range.NumDescriptors = static_cast<UINT>(dx12_render_interface::k_descriptor_table_sizes[(int)type]);

        switch (type)
        {
        case ri_descriptor_table::texture_1d:
        case ri_descriptor_table::texture_2d:
        case ri_descriptor_table::texture_3d:
        case ri_descriptor_table::texture_cube:
        case ri_descriptor_table::buffer:
        case ri_descriptor_table::tlas:
            {
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                break;
            }
        case ri_descriptor_table::sampler:
            {
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                break;
            }
        case ri_descriptor_table::rwbuffer:
        case ri_descriptor_table::rwtexture_2d:
            {
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                break;
            }
        default:
            {
                db_fatal(renderer, "Attempted to bind unsupported descriptor table to root parameter.");
                break;
            }
        }

        D3D12_ROOT_PARAMETER& table = parameters.emplace_back();
        table.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        table.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        table.DescriptorTable.NumDescriptorRanges = 1;
        table.DescriptorTable.pDescriptorRanges = &range;
    }

    // Create root parameters for each param block. This is recommended over putting them in the descriptor heap.
    size_t cbv_index = 0;
    for (size_t i = 0; i < m_create_params.param_block_archetypes.size(); i++)
    {
        // Skip the instance param blocks they are indirectly referenced.
        if (m_create_params.param_block_archetypes[i]->get_create_params().scope == ri_data_scope::instance ||
            m_create_params.param_block_archetypes[i]->get_create_params().scope == ri_data_scope::indirect)
        {
            continue;
        }

        D3D12_ROOT_PARAMETER& table = parameters.emplace_back();
        table.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        table.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        table.Descriptor.RegisterSpace = 0;
        table.Descriptor.ShaderRegister = static_cast<UINT>(cbv_index);

        cbv_index++;
    }

    /*
    D3D12_ROOT_PARAMETER& cbuffer_table = parameters.emplace_back();
    cbuffer_table.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    cbuffer_table.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    cbuffer_table.DescriptorTable.NumDescriptorRanges = 1;
    cbuffer_table.DescriptorTable.pDescriptorRanges = descriptor_ranges.data() + descriptor_ranges.size();

    D3D12_DESCRIPTOR_RANGE& range = descriptor_ranges.emplace_back();
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    range.NumDescriptors = static_cast<UINT>(m_create_params.param_block_archetypes.size());
    range.BaseShaderRegister = 0;
    range.RegisterSpace = 0;
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    */

    // Now tie everything together and serialize our new root sig ...
    D3D12_ROOT_SIGNATURE_DESC root_sig_desc;
    root_sig_desc.NumParameters = static_cast<UINT>(parameters.size());
    root_sig_desc.pParameters = parameters.data();
    root_sig_desc.NumStaticSamplers = 0;
    root_sig_desc.pStaticSamplers = nullptr;
    root_sig_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ID3DBlob* serialized_root_sig = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&root_sig_desc, D3D_ROOT_SIGNATURE_VERSION_1, &serialized_root_sig, nullptr);
    if (FAILED(hr))
    {
        db_error(render_interface, "D3D12SerializeRootSignature failed with error 0x%08x.", hr);
        return false;
    }

    hr = m_renderer.get_device()->CreateRootSignature(0, serialized_root_sig->GetBufferPointer(), serialized_root_sig->GetBufferSize(), IID_PPV_ARGS(&m_root_signature));
    if (FAILED(hr))
    {
        serialized_root_sig->Release();

        db_error(render_interface, "CreateGraphicsPipelineState failed with error 0x%08x.", hr);
        return false;
    }

    serialized_root_sig->Release();
    return true;
}

result<void> dx12_ri_pipeline::create_raytracing_pso()
{
    D3D12_GLOBAL_ROOT_SIGNATURE global_root_signature_desc;
    global_root_signature_desc.pGlobalRootSignature = m_root_signature.Get();

    D3D12_RAYTRACING_SHADER_CONFIG shader_config_desc;
    shader_config_desc.MaxAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;
    shader_config_desc.MaxPayloadSizeInBytes = m_create_params.render_state.max_rt_payload_size;

    D3D12_RAYTRACING_PIPELINE_CONFIG pipeline_config_desc;
    pipeline_config_desc.MaxTraceRecursionDepth = 1;

    std::vector<std::unique_ptr<D3D12_DXIL_LIBRARY_DESC>> library_descs;
    std::vector<std::unique_ptr<D3D12_EXPORT_DESC>> export_descs;
    std::vector<std::unique_ptr<D3D12_HIT_GROUP_DESC>> hit_group_descs;
    std::vector<std::unique_ptr<std::wstring>> export_names;

    std::vector<D3D12_STATE_SUBOBJECT> subobjects;

    auto add_string = [&export_names](const std::string& in) -> const wchar_t*
    {
        std::unique_ptr<std::wstring> export_name = std::make_unique<std::wstring>();
        *export_name = widen_string(in.c_str());

        const wchar_t* ret = export_name->c_str();
        export_names.push_back(std::move(export_name));
        return ret;
    };

    // Add global root signature.
    {
        D3D12_STATE_SUBOBJECT& sub_object = subobjects.emplace_back();
        sub_object.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
        sub_object.pDesc = &global_root_signature_desc;
    }

    // Add RT config.
    {
        D3D12_STATE_SUBOBJECT& sub_object = subobjects.emplace_back();
        sub_object.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
        sub_object.pDesc = &shader_config_desc;
    }
    {
        D3D12_STATE_SUBOBJECT& sub_object = subobjects.emplace_back();
        sub_object.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
        sub_object.pDesc = &pipeline_config_desc;
    }

    // Add all the RT shaders this pipeline uses.
    for (size_t i = (int)ri_shader_stage::rt_start; i <= (int)ri_shader_stage::rt_end; i++)
    {
        ri_pipeline::create_params::stage& stage_params = m_create_params.stages[i];
        if (stage_params.bytecode.empty())
        {
            continue;
        }

        std::unique_ptr<D3D12_EXPORT_DESC> export_desc = std::make_unique<D3D12_EXPORT_DESC>();
        export_desc->Name = add_string(stage_params.entry_point);
        export_desc->Flags = D3D12_EXPORT_FLAG_NONE;
        export_desc->ExportToRename = nullptr;

        std::unique_ptr<D3D12_DXIL_LIBRARY_DESC> sub_object_desc = std::make_unique<D3D12_DXIL_LIBRARY_DESC>();
        sub_object_desc->DXILLibrary.pShaderBytecode = stage_params.bytecode.data();
        sub_object_desc->DXILLibrary.BytecodeLength = stage_params.bytecode.size();
        sub_object_desc->NumExports = 1;
        sub_object_desc->pExports = export_desc.get();

        D3D12_STATE_SUBOBJECT& sub_object = subobjects.emplace_back();
        sub_object.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        sub_object.pDesc = sub_object_desc.get();

        library_descs.push_back(std::move(sub_object_desc));
        export_descs.push_back(std::move(export_desc));
    }

    // Add all the raytrace hitgroups.
    for (ri_pipeline::create_params::ray_hitgroup& hitgroup : m_create_params.ray_hitgroups)
    {
        for (size_t i = (int)ri_shader_stage::rt_start; i <= (int)ri_shader_stage::rt_end; i++)
        {
            ri_pipeline::create_params::stage& stage_params = hitgroup.stages[i];
            if (stage_params.bytecode.empty())
            {
                continue;
            }

            std::unique_ptr<D3D12_EXPORT_DESC> export_desc = std::make_unique<D3D12_EXPORT_DESC>();
            export_desc->Name = add_string(stage_params.entry_point);
            export_desc->Flags = D3D12_EXPORT_FLAG_NONE;
            export_desc->ExportToRename = nullptr;

            std::unique_ptr<D3D12_DXIL_LIBRARY_DESC> sub_object_desc = std::make_unique<D3D12_DXIL_LIBRARY_DESC>();
            sub_object_desc->DXILLibrary.pShaderBytecode = stage_params.bytecode.data();
            sub_object_desc->DXILLibrary.BytecodeLength = stage_params.bytecode.size();
            sub_object_desc->NumExports = 1;
            sub_object_desc->pExports = export_desc.get();

            D3D12_STATE_SUBOBJECT& sub_object = subobjects.emplace_back();
            sub_object.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
            sub_object.pDesc = sub_object_desc.get();

            library_descs.push_back(std::move(sub_object_desc));
            export_descs.push_back(std::move(export_desc));
        }

        std::unique_ptr<D3D12_HIT_GROUP_DESC> hitgroup_desc = std::make_unique<D3D12_HIT_GROUP_DESC>();
        hitgroup_desc->HitGroupExport = add_string(hitgroup.name);
        hitgroup_desc->AnyHitShaderImport = nullptr;
        hitgroup_desc->ClosestHitShaderImport = nullptr;
        hitgroup_desc->IntersectionShaderImport = nullptr;

        if (hitgroup.stages[(int)ri_shader_stage::ray_intersection].bytecode.empty())
        {
            hitgroup_desc->Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;

            if (!hitgroup.stages[(int)ri_shader_stage::ray_any_hit].bytecode.empty())
            {
                hitgroup_desc->AnyHitShaderImport = add_string(hitgroup.stages[(int)ri_shader_stage::ray_any_hit].entry_point);
            }
            if (!hitgroup.stages[(int)ri_shader_stage::ray_closest_hit].bytecode.empty())
            {
                hitgroup_desc->ClosestHitShaderImport = add_string(hitgroup.stages[(int)ri_shader_stage::ray_closest_hit].entry_point);
            }
        }
        else
        {
            hitgroup_desc->Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
            hitgroup_desc->IntersectionShaderImport = add_string(hitgroup.stages[(int)ri_shader_stage::ray_intersection].entry_point);
        }

        D3D12_STATE_SUBOBJECT& sub_object = subobjects.emplace_back();
        sub_object.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
        sub_object.pDesc = hitgroup_desc.get();

        hit_group_descs.push_back(std::move(hitgroup_desc));
    }

    // Add all the raytrace missgroups.
    for (ri_pipeline::create_params::ray_missgroup& missgroup : m_create_params.ray_missgroups)
    {
        ri_pipeline::create_params::stage& stage_params = missgroup.ray_miss_stage;
        if (stage_params.bytecode.empty())
        {
            continue;
        }

        std::unique_ptr<D3D12_EXPORT_DESC> export_desc = std::make_unique<D3D12_EXPORT_DESC>();
        export_desc->Name = add_string(stage_params.entry_point);
        export_desc->Flags = D3D12_EXPORT_FLAG_NONE;
        export_desc->ExportToRename = nullptr;

        std::unique_ptr<D3D12_DXIL_LIBRARY_DESC> sub_object_desc = std::make_unique<D3D12_DXIL_LIBRARY_DESC>();
        sub_object_desc->DXILLibrary.pShaderBytecode = stage_params.bytecode.data();
        sub_object_desc->DXILLibrary.BytecodeLength = stage_params.bytecode.size();
        sub_object_desc->NumExports = 1;
        sub_object_desc->pExports = export_desc.get();

        D3D12_STATE_SUBOBJECT& sub_object = subobjects.emplace_back();
        sub_object.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        sub_object.pDesc = sub_object_desc.get();

        library_descs.push_back(std::move(sub_object_desc));
        export_descs.push_back(std::move(export_desc));
    }

    D3D12_STATE_OBJECT_DESC desc = {};
    desc.NumSubobjects = (UINT)subobjects.size();
    desc.pSubobjects = subobjects.data();
    desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

    HRESULT hr = m_renderer.get_device()->CreateStateObject(&desc, IID_PPV_ARGS(&m_rt_pipeline_state));
    if (FAILED(hr))
    {
        db_error(render_interface, "CreateStateObject failed with error 0x%08x.", hr);
        return false;
    }

    if (result<void> ret = build_sbt(); !ret)
    {
        return false;
    }

    m_is_raytracing = true;
    return true;
}

result<void> dx12_ri_pipeline::build_sbt()
{
    // Create the shader binding table.
    Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> state_properties = nullptr;
    if (HRESULT hr = m_rt_pipeline_state.As(&state_properties); FAILED(hr))
    {
        db_error(render_interface, "Failed to get ID3D12StateObjectProperties with error 0x%08x.", hr);
        return false;
    }

    size_t sbt_hitgroup_count = m_create_params.ray_domain_count * m_create_params.ray_type_count;
    size_t sbt_miss_count = m_create_params.ray_type_count;
    size_t sbt_generate_count = 1;
    size_t sbt_record_count = ((sbt_hitgroup_count * 3) + sbt_miss_count + sbt_generate_count);
    size_t shader_id_size = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    size_t shader_table_size = shader_id_size * sbt_record_count;

    std::vector<uint8_t> sbt_data;
    sbt_data.resize(shader_table_size, 0);

    // Build the SBT's data.
    uint8_t* record_ptr = sbt_data.data();
    bool failed = false;

    auto add_record = [&record_ptr, &state_properties, &failed](const std::string& name, bool optional = false) {
        if (name.empty() && optional)
        {
            record_ptr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
            return;
        }
        void* shader_id = state_properties->GetShaderIdentifier(widen_string(name).c_str());
        if (shader_id == nullptr)
        {
            db_error(render_interface, "Failed to find shader id for shader with entry point '%s'.", name.c_str());
            failed = true;
            return;
        }
        memcpy(record_ptr, shader_id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        record_ptr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    };

    auto add_empty_record = [&record_ptr]() {
        record_ptr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    };

    auto find_hit_group = [this](size_t domain, size_t type) -> ri_pipeline::create_params::ray_hitgroup* {
        for (ri_pipeline::create_params::ray_hitgroup& hitgroup : m_create_params.ray_hitgroups)
        {
            if (hitgroup.domain == domain && hitgroup.type == type)
            {
                return &hitgroup;
            }
        }
        return nullptr;
    };

    auto find_miss_group = [this](size_t type) -> ri_pipeline::create_params::ray_missgroup* {
        for (ri_pipeline::create_params::ray_missgroup& missgroup : m_create_params.ray_missgroups)
        {
            if (missgroup.type == type)
            {
                return &missgroup;
            }
        }
        return nullptr;
    };

    // Generation shader first.
    m_ray_generation_shader_offset = (size_t)(record_ptr - sbt_data.data());
    add_record(m_create_params.stages[(int)ri_shader_stage::ray_generation].entry_point);

    // Miss shaders next.
    m_ray_miss_table_offset = (size_t)(record_ptr - sbt_data.data());
    for (size_t type = 0; type < m_create_params.ray_type_count; type++)
    {
        ri_pipeline::create_params::ray_missgroup* miss_group = find_miss_group(type);
        if (miss_group == nullptr)
        {
            add_empty_record();
        }
        else
        {
            add_record(miss_group->ray_miss_stage.entry_point);
        }
    }

    // Add all the hitgroups.
    m_ray_hit_group_table_offset = (size_t)(record_ptr - sbt_data.data());
    for (size_t domain = 0; domain < m_create_params.ray_domain_count; domain++)
    {
        for (size_t type = 0; type < m_create_params.ray_type_count; type++)
        {
            ri_pipeline::create_params::ray_hitgroup* hit_group = find_hit_group(domain, type);
            if (hit_group == nullptr)
            {
                add_empty_record();
            }
            else
            {
                add_record(hit_group->name);
            }
        }
    }

    if (failed)
    {
        return false;
    }

    ri_buffer::create_params sbt_params;
    sbt_params.element_count = 1;
    sbt_params.element_size = shader_table_size;
    sbt_params.usage = ri_buffer_usage::raytracing_shader_binding_table;
    sbt_params.linear_data = std::span<uint8_t>(sbt_data.begin(), sbt_data.end());

    m_shader_binding_table = m_renderer.create_buffer(sbt_params, string_format("%s : shader binding table", m_debug_name.c_str()).c_str());

    return true;
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE dx12_ri_pipeline::get_hit_group_table()
{
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE ret;
    ret.StartAddress = static_cast<dx12_ri_buffer*>(m_shader_binding_table.get())->get_gpu_address() + m_ray_hit_group_table_offset;
    ret.StrideInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    ret.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES * m_create_params.ray_type_count * m_create_params.ray_domain_count * 3;
    return ret;
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE dx12_ri_pipeline::get_miss_shader_table()
{
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE ret;
    ret.StartAddress = static_cast<dx12_ri_buffer*>(m_shader_binding_table.get())->get_gpu_address() + m_ray_miss_table_offset;
    ret.StrideInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    ret.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES * m_create_params.ray_type_count;
    return ret;
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE dx12_ri_pipeline::get_ray_generation_shader_record()
{
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE ret;
    ret.StartAddress = static_cast<dx12_ri_buffer*>(m_shader_binding_table.get())->get_gpu_address() + m_ray_generation_shader_offset;
    ret.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    return ret;
}

result<void> dx12_ri_pipeline::create_graphics_pso()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};

    // Shader bytecode.

    auto BindShader = [&desc, this](ri_shader_stage stage, D3D12_SHADER_BYTECODE& bytecode)
    {
        size_t stage_index = static_cast<size_t>(stage);
        ri_pipeline::create_params::stage& stage_params = m_create_params.stages[stage_index];
        if (stage_params.bytecode.size() > 0)
        {
            bytecode.BytecodeLength = stage_params.bytecode.size();
            bytecode.pShaderBytecode = stage_params.bytecode.data();
        }
    };

    BindShader(ri_shader_stage::vertex, desc.VS);
    BindShader(ri_shader_stage::pixel, desc.PS);
    BindShader(ri_shader_stage::domain, desc.DS);
    BindShader(ri_shader_stage::hull, desc.HS);
    BindShader(ri_shader_stage::geometry, desc.GS);

    // Blend state

    desc.BlendState.AlphaToCoverageEnable = m_create_params.render_state.alpha_to_coverage;
    desc.BlendState.IndependentBlendEnable = m_create_params.render_state.independent_blend_enabled;

    for (size_t i = 0; i < ri_pipeline_render_state::k_max_output_targets; i++)
    {
        desc.BlendState.RenderTarget[i].BlendEnable = m_create_params.render_state.blend_enabled[i];
        desc.BlendState.RenderTarget[i].BlendOp = ri_to_dx12(m_create_params.render_state.blend_op[i]);
        desc.BlendState.RenderTarget[i].BlendOpAlpha = ri_to_dx12(m_create_params.render_state.blend_alpha_op[i]);
        desc.BlendState.RenderTarget[i].DestBlend = ri_to_dx12(m_create_params.render_state.blend_destination_op[i]);
        desc.BlendState.RenderTarget[i].DestBlendAlpha = ri_to_dx12(m_create_params.render_state.blend_alpha_destination_op[i]);
        desc.BlendState.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_COPY;
        desc.BlendState.RenderTarget[i].LogicOpEnable = false;
        desc.BlendState.RenderTarget[i].RenderTargetWriteMask = 15;
        desc.BlendState.RenderTarget[i].SrcBlend = ri_to_dx12(m_create_params.render_state.blend_source_op[i]);
        desc.BlendState.RenderTarget[i].SrcBlendAlpha = ri_to_dx12(m_create_params.render_state.blend_alpha_source_op[i]);
    }

    // Render state

    desc.RasterizerState.AntialiasedLineEnable = m_create_params.render_state.antialiased_line_enabled;
    desc.RasterizerState.ConservativeRaster = (m_create_params.render_state.conservative_raster_enabled ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);
    desc.RasterizerState.CullMode = ri_to_dx12(m_create_params.render_state.cull_mode);
    desc.RasterizerState.DepthBias = m_create_params.render_state.depth_bias;
    desc.RasterizerState.DepthBiasClamp = m_create_params.render_state.depth_bias_clamp;
    desc.RasterizerState.DepthClipEnable = m_create_params.render_state.depth_clip_enabled;
    desc.RasterizerState.FillMode = ri_to_dx12(m_create_params.render_state.fill_mode);
    desc.RasterizerState.ForcedSampleCount = 0;
    desc.RasterizerState.FrontCounterClockwise = false;
    desc.RasterizerState.MultisampleEnable = m_create_params.render_state.multisample_enabled;
    desc.RasterizerState.SlopeScaledDepthBias = m_create_params.render_state.slope_scaled_depth_bias;

    // Depth state

    desc.DepthStencilState.DepthEnable = m_create_params.render_state.depth_test_enabled;
    desc.DepthStencilState.DepthFunc = ri_to_dx12(m_create_params.render_state.depth_compare_op);
    desc.DepthStencilState.DepthWriteMask = (m_create_params.render_state.depth_write_enabled ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO);
    desc.DepthStencilState.StencilEnable = m_create_params.render_state.stencil_test_enabled;
    desc.DepthStencilState.StencilReadMask = m_create_params.render_state.stencil_read_mask;
    desc.DepthStencilState.StencilWriteMask = m_create_params.render_state.stencil_write_mask;
    desc.DepthStencilState.BackFace.StencilFunc = ri_to_dx12(m_create_params.render_state.stencil_back_face_compare_op);
    desc.DepthStencilState.BackFace.StencilDepthFailOp = ri_to_dx12(m_create_params.render_state.stencil_back_face_depth_fail_op);
    desc.DepthStencilState.BackFace.StencilPassOp = ri_to_dx12(m_create_params.render_state.stencil_back_face_pass_op);
    desc.DepthStencilState.BackFace.StencilFailOp = ri_to_dx12(m_create_params.render_state.stencil_back_face_fail_op);
    desc.DepthStencilState.FrontFace.StencilFunc = ri_to_dx12(m_create_params.render_state.stencil_front_face_compare_op);
    desc.DepthStencilState.FrontFace.StencilDepthFailOp = ri_to_dx12(m_create_params.render_state.stencil_front_face_depth_fail_op);
    desc.DepthStencilState.FrontFace.StencilFailOp = ri_to_dx12(m_create_params.render_state.stencil_front_face_pass_op);
    desc.DepthStencilState.FrontFace.StencilPassOp = ri_to_dx12(m_create_params.render_state.stencil_front_face_fail_op);

    desc.PrimitiveTopologyType = ri_to_dx12(m_create_params.render_state.topology);

    // Vertex layout

    // We use bindless for out vertex data so our input layout can be empty.
    desc.InputLayout.NumElements = 0;
    desc.InputLayout.pInputElementDescs = nullptr;

    // MSAA config

    if (m_create_params.render_state.multisample_enabled)
    {
        desc.SampleDesc.Count = m_create_params.render_state.multisample_count;
        desc.SampleDesc.Quality = DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN;
    }
    else
    {
        desc.SampleDesc.Count = 1;
    }

    // Output configuration.

    desc.NumRenderTargets = static_cast<UINT>(m_create_params.color_formats.size());

    db_assert_message(m_create_params.color_formats.size() < 8, "DX12 only supports up to 8 output color targets.");
    for (size_t i = 0; i < m_create_params.color_formats.size(); i++)
    {
        desc.RTVFormats[i] = ri_to_dx12(m_create_params.color_formats[i]);
    }
    desc.DSVFormat = ri_to_dx12(m_create_params.depth_format);

    // Root signature

    desc.pRootSignature = m_root_signature.Get();

    // Streaming output

    desc.StreamOutput = {};

    // Misc

    desc.SampleMask = UINT_MAX;
    desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    desc.NodeMask = 0;
    desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    // PSO caching

    desc.CachedPSO.CachedBlobSizeInBytes = 0;
    desc.CachedPSO.pCachedBlob = nullptr;

    // Create the actual pipeline state.

    HRESULT hr = m_renderer.get_device()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pipeline_state));
    if (FAILED(hr))
    {
        db_error(render_interface, "CreateGraphicsPipelineState failed with error 0x%08x.", hr);
        return false;
    }

    return true;
}

result<void> dx12_ri_pipeline::create_compute_pso()
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = m_root_signature.Get();

    // Bytecode.

    ri_pipeline::create_params::stage& stage_params = m_create_params.stages[(int)ri_shader_stage::compute];
    if (stage_params.bytecode.size() > 0)
    {
        desc.CS.BytecodeLength = stage_params.bytecode.size();
        desc.CS.pShaderBytecode = stage_params.bytecode.data();
    }

    // PSO caching

    desc.CachedPSO.CachedBlobSizeInBytes = 0;
    desc.CachedPSO.pCachedBlob = nullptr;

    // Misc

    desc.NodeMask = 0;
    desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    // Create the actual pipeline state.

    HRESULT hr = m_renderer.get_device()->CreateComputePipelineState(&desc, IID_PPV_ARGS(&m_pipeline_state));
    if (FAILED(hr))
    {
        db_error(render_interface, "CreateComputePipelineState failed with error 0x%08x.", hr);
        return false;
    }

    m_is_compute = true;

    return true;
}

result<void> dx12_ri_pipeline::create_resources()
{
    // Create root signature up front.    
    if (!create_root_signature())
    {
        return false;
    }

    // Generate the PSO based on what pipeline type we are.
    result<void> ret = false;
    if (!m_create_params.stages[(int)ri_shader_stage::ray_generation].bytecode.empty() ||
        !m_create_params.stages[(int)ri_shader_stage::ray_intersection].bytecode.empty() ||
        !m_create_params.stages[(int)ri_shader_stage::ray_any_hit].bytecode.empty() ||
        !m_create_params.stages[(int)ri_shader_stage::ray_closest_hit].bytecode.empty() ||
        !m_create_params.stages[(int)ri_shader_stage::ray_miss].bytecode.empty())
    {
        ret = create_raytracing_pso();
    }
    else if (!m_create_params.stages[(int)ri_shader_stage::compute].bytecode.empty())
    {
        ret = create_compute_pso();
    }
    else
    {
        ret = create_graphics_pso();
    }

    // Clear bytecode data, we don't need it any longer.
    for (auto& stage : m_create_params.stages)
    {
        stage.bytecode.clear();
    }

    return ret;
}

bool dx12_ri_pipeline::is_compute()
{
    return m_is_compute;   
}

bool dx12_ri_pipeline::is_raytracing()
{
    return m_is_raytracing;
}

ID3D12PipelineState* dx12_ri_pipeline::get_pipeline_state()
{
    return m_pipeline_state.Get();
}

ID3D12StateObject* dx12_ri_pipeline::get_rt_pipeline_state()
{
    return m_rt_pipeline_state.Get();
}

ID3D12RootSignature* dx12_ri_pipeline::get_root_signature()
{ 
    return m_root_signature.Get();
}

const ri_pipeline::create_params& dx12_ri_pipeline::get_create_params()
{
    return m_create_params;
}

}; // namespace ws
