// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_staging_buffer.h"
#include "workshop.render_interface/ri_command_list.h"
#include "workshop.render_interface.dx12/dx12_ri_descriptor_table.h"
#include "workshop.render_interface.dx12/dx12_ri_small_buffer_allocator.h"
#include "workshop.render_interface.dx12/dx12_ri_upload_manager.h"
#include "workshop.core/utils/result.h"
#include "workshop.render_interface.dx12/dx12_headers.h"
#include "workshop.core/memory/async_copy_manager.h"
#include <array>
#include <string>

namespace ws {

class engine;
class dx12_render_interface;
class dx12_ri_upload_manager;

// ================================================================================================
//  Implementation of a staging buffer using DirectX 12.
// ================================================================================================
class dx12_ri_staging_buffer : public ri_staging_buffer
{
public:
    dx12_ri_staging_buffer(dx12_render_interface& renderer, dx12_ri_upload_manager& upload_manager, const ri_staging_buffer::create_params& params, std::span<uint8_t> data);
    virtual ~dx12_ri_staging_buffer();

    result<void> create_resources();

    virtual bool is_staged() override;
    virtual void wait() override;

private:
    friend class dx12_ri_upload_manager;

    dx12_ri_upload_manager::upload_state take_upload_state();

private:
    dx12_render_interface& m_renderer;
    dx12_ri_upload_manager& m_upload_manager;

    ri_staging_buffer::create_params m_create_params;

    dx12_ri_upload_manager::upload_state m_upload;
    bool m_used = false;

    std::vector<async_copy_request> m_requests;

    std::span<uint8_t> m_data;

};

}; // namespace ws
