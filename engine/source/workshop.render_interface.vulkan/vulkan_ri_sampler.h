// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_sampler.h"
#include "workshop.core/utils/result.h"
#include "workshop.render_interface.dx12/dx12_headers.h"
#include "workshop.render_interface.dx12/dx12_ri_descriptor_table.h"
#include <array>
#include <string>

namespace ws {

class engine;
class dx12_render_interface;

// ================================================================================================
//  Implementation of a texture sampler using DirectX 12.
// ================================================================================================
class dx12_ri_sampler : public ri_sampler
{
public:
    dx12_ri_sampler(dx12_render_interface& renderer, const char* debug_name, const ri_sampler::create_params& params);
    virtual ~dx12_ri_sampler();

    result<void> create_resources();

	virtual ri_texture_filter get_filter() override;
	virtual ri_texture_address_mode get_address_mode_u() override;
	virtual ri_texture_address_mode get_address_mode_v() override;
	virtual ri_texture_address_mode get_address_mode_w() override;
	virtual ri_texture_border_color get_border_color() override;
	virtual float get_min_lod() override;
	virtual float get_max_lod() override;
	virtual float get_mip_lod_bias() override;
	virtual int get_max_anisotropy() override;

public:
	size_t get_descriptor_table_index() const;

private:
    dx12_render_interface& m_renderer;
    std::string m_debug_name;
    ri_sampler::create_params m_create_params;

    dx12_ri_descriptor_table::allocation m_handle;

};

}; // namespace ws
