// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/render_system.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.render_interface/ri_texture.h"
#include "workshop.render_interface/ri_buffer.h"

namespace ws {

class renderer;
class render_light;

// Should match the values in the lighting shader.
enum class render_light_type
{
    directional = 0,
    point = 1,
    spotlight = 2,
};

// ================================================================================================
//  Responsible for clustering, accumulating and applying lighting.
// ================================================================================================
class render_system_lighting
    : public render_system
{
public:
    render_system_lighting(renderer& render);

    virtual void register_init(init_list& list) override;
    virtual void build_pre_graph(render_graph& graph, const render_world_state& state) override;
    virtual void build_graph(render_graph& graph, const render_world_state& state, render_view& view) override;
    virtual void step(const render_world_state& state) override;

    ri_texture& get_lighting_buffer();

    ri_param_block& get_resolve_param_block(render_view& view);

public:

    // Maximum number of lights we can handle on screen at once.
    static inline constexpr size_t k_max_lights = 10000;

    // Maximum number of shadow maps we can handle on screen at once.
    static inline constexpr size_t k_max_shadow_maps = 10000;

    // Number of clusters in each dimension.
    static inline constexpr size_t k_cluster_dimensions = 32;

    // Total number of clusters in all dimensions.
    static inline constexpr size_t k_total_clusters = k_cluster_dimensions * k_cluster_dimensions * k_cluster_dimensions;

    /*
    struct cluster
    {
        frustum bounds;
        std::vector<render_light*> lights;
    };

    struct cluster_list
    {
        std::array<cluster, k_total_clusters> clusters;

        inline cluster& get(size_t x, size_t y, size_t z)
        {
            size_t cluster_per_depth = (k_cluster_dimensions * k_cluster_dimensions);
            size_t cluster_index = (cluster_per_depth * z) + (k_cluster_dimensions * y) + x;
            return clusters[cluster_index];
        }
    };
    */

private:
    result<void> create_resources();
    result<void> destroy_resources();

    //void cluster_lights(render_view& view, const std::vector<render_light*>& lights, cluster_list& clusters);

    void get_cluster_values(vector3u& out_grid_size, size_t& out_cluster_size, size_t& out_max_lights_per_cluster);

private:

    std::unique_ptr<ri_texture> m_brdf_lut_texture;
    bool m_calculated_brdf_lut = false;

    std::unique_ptr<ri_texture> m_lighting_buffer;
    std::unique_ptr<ri_buffer> m_light_cluster_buffer;
    std::unique_ptr<ri_buffer> m_light_cluster_visibility_buffer;
    std::unique_ptr<ri_buffer> m_light_cluster_visibility_count_buffer;
    render_output m_lighting_output;

    frustum m_cluster_prime_frustum;
    float m_cluster_prime_near_z;
    float m_cluster_prime_far_z;

};

}; // namespace ws
