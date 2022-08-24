// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.core/debug/debug.h"

namespace ws {

D3D12_RESOURCE_STATES to_dx12_resource_state(render_resource_state input)
{
    switch (input)
    {
        case render_resource_state::present:       return D3D12_RESOURCE_STATE_PRESENT;
        case render_resource_state::render_target: return D3D12_RESOURCE_STATE_RENDER_TARGET;
        default:
        {
            db_assert_message(false, "Attempt to convert unknown resource state: %i", static_cast<int>(input));
            return D3D12_RESOURCE_STATE_COMMON;
        }
    }
}

}; // namespace ws
