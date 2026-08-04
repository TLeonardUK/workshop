// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_query.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_command_queue.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.window_interface/window.h"

namespace ws {

dx12_ri_query::dx12_ri_query(dx12_render_interface& renderer, const char* debug_name, const ri_query::create_params& params)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
    , m_create_params(params)
{
}

dx12_ri_query::~dx12_ri_query()
{
    m_renderer.get_query_manager().delete_query(m_query_id);
}

result<void> dx12_ri_query::create_resources()
{
    m_query_id = m_renderer.get_query_manager().new_query(m_create_params.type);

    return true;
}

const char* dx12_ri_query::get_debug_name()
{
    return m_debug_name.c_str();
}

bool dx12_ri_query::are_results_ready()
{
    return m_renderer.get_query_manager().are_results_ready(m_query_id);
}

double dx12_ri_query::get_results()
{
    return m_renderer.get_query_manager().get_result(m_query_id);
}

void dx12_ri_query::begin(ID3D12GraphicsCommandList* command_list)
{
    m_renderer.get_query_manager().start_query(m_query_id, command_list);
}

void dx12_ri_query::end(ID3D12GraphicsCommandList* command_list)
{
    m_renderer.get_query_manager().end_query(m_query_id, command_list);
}

}; // namespace ws
