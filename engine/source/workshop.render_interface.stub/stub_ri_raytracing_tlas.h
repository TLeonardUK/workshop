// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_raytracing_tlas.h"
#include "workshop.render_interface.stub/stub_ri_buffer.h"

#include <memory>

namespace ws {

// ================================================================================================
//  Stub implementation of a top level acceleration structure, performs no actual work.
// ================================================================================================
class stub_ri_raytracing_tlas : public ri_raytracing_tlas
{
public:
    stub_ri_raytracing_tlas();

    virtual instance_id add_instance(ri_raytracing_blas* blas, const matrix4& transform, size_t domain, bool opaque, ri_param_block* metadata, uint32_t mask) override;
    virtual void remove_instance(instance_id id) override;
    virtual void update_instance(instance_id id, const matrix4& transform, uint32_t mask) override;

    virtual ri_buffer* get_metadata_buffer() const override;

private:
    std::unique_ptr<stub_ri_buffer> m_metadata_buffer;
    instance_id m_next_instance_id = 1;

};

}; // namespace ws
