// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_command_list.h"
#include "workshop.render_interface/ri_types.h"
#include "workshop.render_interface.dx12/dx12_headers.h"

#include "workshop.core/drawing/color.h"

#include <array>
#include <string>

namespace ws {

// Handy macro for releasing com-ptr objects.
#define SafeRelease(x) if ((x) != nullptr) { (x)->Release(); (x) = nullptr; }

// Handy macro for release windows-handle objects.
#define SafeCloseHandle(x) if ((x) != INVALID_HANDLE_VALUE) { CloseHandle((x)); (x) = INVALID_HANDLE_VALUE; }

// Handy macro for releasing com-ptr objects.
#define CheckedRelease(x) if ((x) != nullptr) { (x)->Release(); }

// Handy macro for release windows-handle objects.
#define CheckedCloseHandle(x) if ((x) != INVALID_HANDLE_VALUE) { CloseHandle((x)); }

// Convert various RI types to their respective DX12 types.
D3D12_RESOURCE_STATES ri_to_dx12(ri_resource_state value);
D3D12_PRIMITIVE_TOPOLOGY_TYPE ri_to_dx12(ri_topology value);
D3D12_FILL_MODE ri_to_dx12(ri_fill_mode value);
D3D12_CULL_MODE ri_to_dx12(ri_cull_mode value);
D3D12_BLEND_OP ri_to_dx12(ri_blend_op value);
D3D12_BLEND ri_to_dx12(ri_blend_operand value);
D3D12_COMPARISON_FUNC ri_to_dx12(ri_compare_op value);
D3D12_STENCIL_OP ri_to_dx12(ri_stencil_op value);
DXGI_FORMAT ri_to_dx12(ri_texture_format value);
D3D12_FILTER ri_to_dx12(ri_texture_filter value);
D3D12_TEXTURE_ADDRESS_MODE ri_to_dx12(ri_texture_address_mode value);
color ri_to_dx12(ri_texture_border_color value);
D3D12_RESOURCE_DIMENSION ri_to_dx12(ri_texture_dimension value);
D3D12_PRIMITIVE_TOPOLOGY ri_to_dx12(ri_primitive value);

}; // namespace ws
