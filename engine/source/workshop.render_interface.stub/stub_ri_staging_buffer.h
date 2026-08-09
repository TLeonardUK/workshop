// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_staging_buffer.h"

namespace ws {

// ================================================================================================
//  Stub implementation of a staging buffer, performs no actual work.
// ================================================================================================
class stub_ri_staging_buffer : public ri_staging_buffer
{
public:
    stub_ri_staging_buffer(const create_params& params, std::span<uint8_t> linear_data);

    virtual bool is_staged() override;
    virtual void wait() override;

private:
    create_params m_params;

};

}; // namespace ws
