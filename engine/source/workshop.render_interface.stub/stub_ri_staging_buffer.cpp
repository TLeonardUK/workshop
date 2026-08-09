// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.stub/stub_ri_staging_buffer.h"

namespace ws {

stub_ri_staging_buffer::stub_ri_staging_buffer(const create_params& params, std::span<uint8_t> linear_data)
    : m_params(params)
{
}

bool stub_ri_staging_buffer::is_staged()
{
    return true;
}

void stub_ri_staging_buffer::wait()
{
}

}; // namespace ws
