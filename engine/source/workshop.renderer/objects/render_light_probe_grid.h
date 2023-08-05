// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/render_object.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface/ri_param_block.h"

namespace ws {

// ================================================================================================
//  Represets a grid of diffuse light probes that the gpu can use for indirect lighting.
// ================================================================================================
class render_light_probe_grid : public render_object
{
public:
    struct probe
    {
        size_t index;
        vector3 origin;
        vector3 extents;
        quat orientation;
        bool dirty;

        std::unique_ptr<ri_param_block> debug_param_block;
    };

public:
    render_light_probe_grid(render_object_id id, renderer& renderer);
    virtual ~render_light_probe_grid();

    // Gets/sets the density of the grid as a value that represents the distance between each probe.
    void set_density(float value);
    float get_density();

    // Overrides the default bounds to return the obb of the model bounds.
    virtual obb get_bounds() override;

    // Gets a list of all probes in the volume.
    std::vector<probe>& get_probes();

    // Called when the bounds of an object is modified.
    virtual void bounds_modified();

    // Gets buffer containing spherical harmonics for each probe inside the grid.
    ri_buffer& get_spherical_harmonic_buffer();

    // Gets the param block describing this grid for rendering it as part of the lighting pass.
    ri_param_block& get_param_block();

    // 9 coefficients for each color channel.
    static inline constexpr size_t k_probe_coefficient_size = 9 * 3 * sizeof(float); 

private:
    void recalculate_probes();

    size_t get_probe_index(size_t x, size_t y, size_t z);

private:
    float m_density = 100.0f;

    // Dimensions in probe cells.
    size_t m_width;
    size_t m_height;
    size_t m_depth;

    std::vector<probe> m_probes;

    std::unique_ptr<ri_buffer> m_spherical_harmonic_buffer;
    std::unique_ptr<ri_param_block> m_param_block;

};

}; // namespace ws
