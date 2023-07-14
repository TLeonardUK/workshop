// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_query.h"
#include "workshop.render_interface/ri_command_list.h"
#include "workshop.render_interface.dx12/dx12_ri_descriptor_table.h"
#include "workshop.core/utils/result.h"
#include "workshop.render_interface.dx12/dx12_headers.h"
#include "workshop.render_interface.dx12/dx12_ri_query_manager.h"
#include <array>
#include <string>

namespace ws {

class engine;
class dx12_render_interface;

// ================================================================================================
//  Implementation of a gpu query using DirectX 12.
// ================================================================================================
class dx12_ri_query : public ri_query
{
public:
    dx12_ri_query(dx12_render_interface& renderer, const char* debug_name, const ri_query::create_params& params);
    virtual ~dx12_ri_query();

    result<void> create_resources();

    virtual const char* get_debug_name() override;
    virtual bool are_results_ready() override;
    virtual double get_results() override;

    void begin(ID3D12GraphicsCommandList* command_list);
    void end(ID3D12GraphicsCommandList* command_list);

private:
    dx12_render_interface& m_renderer;
    std::string m_debug_name;
    ri_query::create_params m_create_params;

    dx12_ri_query_manager::query_id m_query_id;

};

}; // namespace ws
