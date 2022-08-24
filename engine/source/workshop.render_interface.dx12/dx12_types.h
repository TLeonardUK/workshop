// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/render_command_list.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <array>
#include <string>

namespace ws {

// Handy macro for releasing com-ptr objects.
#define SafeRelease(x) if ((x) != nullptr) { (x)->Release(); (x) = nullptr; }

// Handy macro for release windows-handle objects.
#define SafeCloseHandle(x) if ((x) != INVALID_HANDLE_VALUE) { CloseHandle((x)); (x) = INVALID_HANDLE_VALUE; }

// Converts our render interface resource state to a DX12 native version.
D3D12_RESOURCE_STATES to_dx12_resource_state(render_resource_state input);

}; // namespace ws
