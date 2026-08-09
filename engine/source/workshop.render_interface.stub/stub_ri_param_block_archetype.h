// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_param_block_archetype.h"

#include <memory>
#include <string>

namespace ws {

// ================================================================================================
//  Stub implementation of a param block archetype, performs no actual work.
// ================================================================================================
class stub_ri_param_block_archetype : public ri_param_block_archetype
{
public:
    stub_ri_param_block_archetype(const create_params& params, const char* debug_name);

    virtual std::unique_ptr<ri_param_block> create_param_block() override;

    virtual const create_params& get_create_params() override;
    virtual const char* get_name() override;

    virtual size_t get_size() override;

private:
    create_params m_params;
    std::string m_debug_name;

};

}; // namespace ws
