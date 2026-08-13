// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.stub/stub_ri_raytracing_tlas.h"

namespace ws {

namespace {

ri_buffer::create_params make_metadata_buffer_params()
{
    ri_buffer::create_params params;
    params.usage = ri_buffer_usage::raytracing_as_instance_data;
    return params;
}

}; // namespace

stub_ri_raytracing_tlas::stub_ri_raytracing_tlas()
    : m_metadata_buffer(std::make_unique<stub_ri_buffer>(make_metadata_buffer_params(), "Stub TLAS Metadata Buffer"))
{
}

ri_raytracing_tlas::instance_id stub_ri_raytracing_tlas::add_instance(ri_raytracing_blas* blas, const matrix4& transform, size_t domain, bool opaque, ri_param_block* metadata, uint32_t mask)
{
    return m_next_instance_id++;
}

void stub_ri_raytracing_tlas::remove_instance(instance_id id)
{
}

void stub_ri_raytracing_tlas::update_instance(instance_id id, const matrix4& transform, uint32_t mask)
{
}

ri_buffer* stub_ri_raytracing_tlas::get_metadata_buffer() const
{
    return m_metadata_buffer.get();
}

}; // namespace ws
