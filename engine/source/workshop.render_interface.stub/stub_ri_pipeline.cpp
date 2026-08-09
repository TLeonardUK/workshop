// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.stub/stub_ri_pipeline.h"

namespace ws {

stub_ri_pipeline::stub_ri_pipeline(const create_params& params, const char* debug_name)
    : m_params(params)
{
}

const ri_pipeline::create_params& stub_ri_pipeline::get_create_params()
{
    return m_params;
}

}; // namespace ws
