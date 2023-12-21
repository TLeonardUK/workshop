// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_cvars.h"
#include "workshop.render_interface/ri_interface.h"

namespace ws {

void register_render_cvars(ri_interface& ri_interface)
{
    // Set some hardware-specific values before registering everything.
    size_t vram_local_total;
    size_t vram_non_local_total;
    ri_interface.get_vram_total(vram_local_total, vram_non_local_total);
    cvar_gpu_memory.set_variant((int)(vram_local_total / (1024 * 1024)), cvar_source::set_by_code_default, true);

    cvar_gpu_memory.register_self();
    cvar_texture_detail.register_self();

    cvar_texture_streaming_enabled.register_self();
    cvar_textures_dropped_mips.register_self();
    cvar_texture_streaming_max_resident_mips.register_self();
    cvar_texture_streaming_min_resident_mips.register_self();
    cvar_texture_streaming_min_dimension.register_self();
    cvar_texture_streaming_pool_size.register_self();
    cvar_texture_streaming_force_unstream.register_self();
    cvar_texture_streaming_time_limit_ms.register_self();
    cvar_texture_streaming_max_staged_memory.register_self();
    cvar_texture_streaming_mip_bias.register_self();

    cvar_light_probe_ray_count.register_self();
    cvar_light_probe_max_regenerations_per_frame.register_self();
    cvar_light_probe_far_z.register_self();
    cvar_light_probe_queue_update_distance.register_self();
    cvar_light_probe_distance_exponent.register_self();
    cvar_light_probe_regeneration_time_limit_ms.register_self();
    cvar_light_probe_regeneration_step_amount.register_self();
    cvar_light_probe_normal_bias.register_self();
    cvar_light_probe_view_bias.register_self();
    cvar_light_probe_blend_hysteresis.register_self();
    cvar_light_probe_large_change_threshold.register_self();
    cvar_light_probe_brightness_threshold.register_self();
    cvar_light_probe_fixed_ray_backface_threshold.register_self();
    cvar_light_probe_random_ray_backface_threshold.register_self();
    cvar_light_probe_min_frontface_distance.register_self();
    cvar_light_probe_encoding_gamma.register_self();

    cvar_reflection_probe_cubemap_size.register_self();
    cvar_reflection_probe_cubemap_mip_count.register_self();
    cvar_reflection_probe_max_regenerations_per_frame.register_self();
    cvar_reflection_probe_near_z.register_self();
    cvar_reflection_probe_far_z.register_self();

    cvar_lighting_max_lights.register_self();
    cvar_lighting_max_shadow_maps.register_self();

    cvar_shadows_max_cascade_updates_per_frame.register_self();

    cvar_eye_adapation_min_luminance.register_self();
    cvar_eye_adapation_max_luminance.register_self();
    cvar_eye_adapation_white_point.register_self();
    cvar_eye_adapation_exposure_tau.register_self();

    cvar_ssao_enabled.register_self();
    cvar_ssao_sample_radius.register_self();
    cvar_ssao_intensity_power.register_self();
    cvar_ssao_resolution_scale.register_self();
    cvar_ssao_direct_light_effect.register_self();

    cvar_raytracing_enabled.register_self();
}

}; // namespace ws
