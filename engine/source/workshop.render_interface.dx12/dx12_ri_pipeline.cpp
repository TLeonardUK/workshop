// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_pipeline.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_types.h"

namespace ws {

dx12_ri_pipeline::dx12_ri_pipeline(dx12_render_interface& renderer, const dx12_ri_pipeline::create_params& params, const char* debug_name)
    : m_renderer(renderer)
    , m_create_params(params)
    , m_debug_name(debug_name)
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

        switch (type)
        {
        case ri_descriptor_table::texture_1d:
        case ri_descriptor_table::texture_2d:
        case ri_descriptor_table::texture_3d:
        case ri_descriptor_table::texture_cube:
            {
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                range.NumDescriptors = dx12_render_interface::k_srv_descriptor_table_size;
                break;
            }
        case ri_descriptor_table::sampler:
            {
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                range.NumDescriptors = dx12_render_interface::k_sampler_descriptor_table_size;
                break;
            }
        case ri_descriptor_table::buffer:
            {
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                range.NumDescriptors = dx12_render_interface::k_srv_descriptor_table_size;
                break;
            }
        case ri_descriptor_table::rwbuffer:
            {
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                range.NumDescriptors = dx12_render_interface::k_uav_descriptor_table_size;
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
        if (m_create_params.param_block_archetypes[i]->get_create_params().scope == ri_data_scope::instance)
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
    desc.BlendState.IndependentBlendEnable = false;
    desc.BlendState.RenderTarget->BlendEnable = m_create_params.render_state.blend_enabled;
    desc.BlendState.RenderTarget->BlendOp = ri_to_dx12(m_create_params.render_state.blend_op);
    desc.BlendState.RenderTarget->BlendOpAlpha = ri_to_dx12(m_create_params.render_state.blend_alpha_op);
    desc.BlendState.RenderTarget->DestBlend = ri_to_dx12(m_create_params.render_state.blend_destination_op);
    desc.BlendState.RenderTarget->DestBlendAlpha = ri_to_dx12(m_create_params.render_state.blend_alpha_destination_op);
    desc.BlendState.RenderTarget->LogicOp = D3D12_LOGIC_OP_COPY;
    desc.BlendState.RenderTarget->LogicOpEnable = false;
    desc.BlendState.RenderTarget->RenderTargetWriteMask = 15;
    desc.BlendState.RenderTarget->SrcBlend = ri_to_dx12(m_create_params.render_state.blend_source_op);
    desc.BlendState.RenderTarget->SrcBlendAlpha = ri_to_dx12(m_create_params.render_state.blend_alpha_source_op);

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

    m_is_compute = false;

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
    if (!m_create_params.stages[(int)ri_shader_stage::compute].bytecode.empty())
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

ID3D12PipelineState* dx12_ri_pipeline::get_pipeline_state()
{
    return m_pipeline_state.Get();
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
