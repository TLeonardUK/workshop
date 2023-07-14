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
#include <array>
#include <string>

namespace ws {

class engine;
class dx12_render_interface;

// ================================================================================================
//  Handles management and resolving of query data.
// ================================================================================================
class dx12_ri_query_manager
{
public:
    using query_id = size_t;

    static inline const size_t k_invalid_query_id = std::numeric_limits<size_t>::max();

public:
    dx12_ri_query_manager(dx12_render_interface& renderer, size_t max_queries);
    virtual ~dx12_ri_query_manager();

    result<void> create_resources();

    query_id new_query(ri_query_type type);
    void delete_query(query_id id);

    bool are_results_ready(query_id id);
    double get_result(query_id id);

    void start_query(query_id id, ID3D12GraphicsCommandList* list);
    void end_query(query_id id, ID3D12GraphicsCommandList* list);

    void begin_frame();

private:
    dx12_render_interface& m_renderer;
    size_t m_max_queries;
    size_t m_query_slots;
    size_t m_pipeline_depth;

    struct query_info
    {
        ri_query_type type;
        size_t started_frame = 0;
    };

    std::mutex m_mutex;

    std::vector<query_info> m_query_info;

    std::vector<query_id> m_free_queries;
    std::vector<uint64_t> m_read_back_times;

    Microsoft::WRL::ComPtr<ID3D12QueryHeap> m_query_heap = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_read_back_buffer = nullptr;

    size_t m_resolve_frame_index = 0;

    UINT64 m_timestamp_frequency = 0;
    double m_timestamp_frequency_inv = 0.0;

};

}; // namespace ws
