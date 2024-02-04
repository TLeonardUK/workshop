// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/cvar/cvar.h"
#include "workshop.core/platform/platform.h"
#include "workshop.render_interface/ri_interface.h"

namespace ws {

void register_render_cvars(ri_interface& ri_interface);

// ================================================================================================
//  Read-Only properties used for configuration files.
// ================================================================================================

inline cvar<int> cvar_gpu_memory(
    cvar_flag::read_only,
    0,
    "gpu_memory",
    "Number of megabytes of dedicated gpu memory."
);

// ================================================================================================
//  High level configuration values, config files use these values to configure the rest
//  of the settings.
// ================================================================================================

inline cvar<int> cvar_texture_detail(
    cvar_flag::saved | cvar_flag::machine_specific | cvar_flag::evaluate_on_change, 
    0, 
    "texture_detail", 
    "Determines the quality level of textures and various streaming settings. 0 is low quality, 3 is high."
);

// ================================================================================================
//  Textures
// ================================================================================================

inline cvar<bool> cvar_texture_streaming_enabled(
    cvar_flag::none, 
    true, 
    "texture_streaming_enabled", 
    "Toggles texture streaming on/off."
);

inline cvar<int> cvar_textures_dropped_mips(
    cvar_flag::none,
    0,
    "textures_dropped_mips",
    "How many mips to drop of a texture as its loaded. This can be used to quickly strim down the maximum memory being used. In general texture streaming/etc should be used rather than this."
);

inline cvar<int> cvar_texture_streaming_max_resident_mips(
    cvar_flag::none,
    99999,
    "texture_streaming_max_resident_mips",
    "Maximum number of mips that can be resident in a texture. In general leave this uncapped. If you want to force the streamer to drop top level mips consider using the texture_dropped_mips value instead."
);

inline cvar<int> cvar_texture_streaming_min_resident_mips(
    cvar_flag::none,
    5, // ~64x64
    "texture_streaming_min_resident_mips",
    "Minimum number of mips that can be resident in a texture. Ideally this should be set to the maximum number of mips that fit into a memory page. Any less and you save no memory but waste streaming time and potentially cause other issues."
);

inline cvar<int> cvar_texture_streaming_min_dimension(
	cvar_flag::none,
	128,
    "texture_streaming_min_dimension",
    "Minimum dimension of a texture for it to be considered for streaming."
);

inline cvar<int> cvar_texture_streaming_pool_size(
	cvar_flag::none,
	1024,
    "texture_streaming_pool_size",
    "Maximum size of the streamed texture pool in megabytes."
);

inline cvar<bool> cvar_texture_streaming_force_unstream(
	cvar_flag::none,
	false,
    "texture_streaming_force_unstream",
    "Forces texture mips to be unstreamed even when not under memory pressure. Useful for debugging, unwise to use in production."
);

inline cvar<float> cvar_texture_streaming_time_limit_ms(
	cvar_flag::none,
	1.0f,
    "texture_streaming_time_limit_ms",
    "Maximum number of ms per frame to spend on the render thread making mips resident. Mips will be spread across frames if this time limit is exceeded."
);

inline cvar<int> cvar_texture_streaming_max_staged_memory(
	cvar_flag::none,
	64,
    "texture_streaming_max_staged_memory",
    "Maximum amount of memory that should be used for staging buffers at any given time, in megabytes. Contrains number of mips that can be concurrently staged. Reduces memory and processing spikes."
);

inline cvar<int> cvar_texture_streaming_mip_bias(
	cvar_flag::none,
	0,
    "texture_streaming_mip_bias",
    "Biases the ideal mip for textures higher or lower than what was calculated."
);

// ================================================================================================
//  Light Probes
// ================================================================================================

inline cvar<int> cvar_light_probe_ray_count(
	cvar_flag::none,
	256,
    "light_probe_ray_count",
    "How many rays to cast per probe to calculate diffus lighting."
);

inline cvar<int> cvar_light_probe_max_regenerations_per_frame(
	cvar_flag::none,
	8192,
    "light_probe_max_regenerations_per_frame",
    "How many probes can be regenerated per frame. Each regeneration can cost as much as an entire scene render, so keep limited to remain responsive."
);

inline cvar<float> cvar_light_probe_far_z(
	cvar_flag::none,
	100'000.0f,
    "light_probe_far_z",
    "Far clipping plane of the view used to capture a light probes cubemap."
);

inline cvar<float> cvar_light_probe_queue_update_distance(
	cvar_flag::none,
	1'000.0f,
    "light_probe_queue_update_distance",
    "Light probes are prioritized for rendering based on how close they are to a normal view. The prioritization list is only updated whenever the view moves by this amount."
);

inline cvar<float> cvar_light_probe_distance_exponent(
	cvar_flag::none,
	50.0f,
    "light_probe_distance_exponent",
    "Exponent used for depth testing. High values react quickly to depth discontinuties but may cause banding."
);

inline cvar<float> cvar_light_probe_regeneration_time_limit_ms(
	cvar_flag::none,
	2.0f,
    "light_probe_regeneration_time_limit_ms",
    "How many milliseconds per frame can be spent on regenerating light probes. As many probes as possible will be run as possible within this time limit up to the maximum."
);

inline cvar<int> cvar_light_probe_regeneration_step_amount(
	cvar_flag::none,
	30,
    "light_probe_regeneration_step_amount",
    "Number of probes per frame to increase or decreate by to adjust to meet the time limit above."
);

inline cvar<float> cvar_light_probe_normal_bias(
	cvar_flag::none,
	10.0f,
    "light_probe_normal_bias",
    "Offset along surface normal applied to shader surface to avoid numeric instability when calculating occlusion."
);

inline cvar<float> cvar_light_probe_view_bias(
	cvar_flag::none,
	60.0f,
    "light_probe_view_bias",
    "Offset along camera view ray applied to shader surface to avoid numeric instability when calculating occlusion."
);

inline cvar<float> cvar_light_probe_blend_hysteresis(
	cvar_flag::none,
	0.97f,
    "light_probe_blend_hysteresis",
    "Speed at which new changes to irradiance are blended into the current value."
);

inline cvar<float> cvar_light_probe_large_change_threshold(
	cvar_flag::none,
	0.35f,
    "light_probe_large_change_threshold",
    "What delta of change in irradiance is considered large and should be blended in faster."
);

inline cvar<float> cvar_light_probe_brightness_threshold(
	cvar_flag::none,
	0.5f,
    "light_probe_brightness_threshold",
    "How much of a brightness change per frame is considered large and should be blended in more slowly."
);

inline cvar<float> cvar_light_probe_fixed_ray_backface_threshold(
	cvar_flag::none,
	0.25f,
    "light_probe_fixed_ray_backface_threshold",
    "Used by probe relocation to determine if a probe is inside geometry if more than this proportion of the rays hit backfaces."
);

inline cvar<float> cvar_light_probe_random_ray_backface_threshold(
	cvar_flag::none,
	0.1f,
    "light_probe_random_ray_backface_threshold",
    "Used during tracing irradiance to determine if a probe is inside geometry if more than this proportion of the rays hit backfaces."
);

inline cvar<float> cvar_light_probe_min_frontface_distance(
	cvar_flag::none,
	50.0f,
    "light_probe_min_frontface_distance",
    "Minimum distance all probes will attempt to keep from frontfacing triangles. This is limited to half the distance between probes in the grid. "
);

inline cvar<float> cvar_light_probe_encoding_gamma(
	cvar_flag::none,
	5.0f,
    "light_probe_encoding_gamma",
    "Tone mapped gamma that light probe blending is performed with."
);

// ================================================================================================
//  Reflection Probes
// ================================================================================================

inline cvar<int> cvar_reflection_probe_cubemap_size(
	cvar_flag::none,
	512,
    "reflection_probe_cubemap_size",
    "Size of the cubemap that is captured for reflection"
);

inline cvar<int> cvar_reflection_probe_cubemap_mip_count(
	cvar_flag::none,
	10,
    "reflection_probe_cubemap_mip_count",
    "Number of mips each reflection probe generates at varying levels of roughness."
);

inline cvar<int> cvar_reflection_probe_max_regenerations_per_frame(
	cvar_flag::none,
	1,
    "reflection_probe_max_regenerations_per_frame",
    "How many probes can be regenerated per frame. Each regeneration can cost as much as an entire scene render, so keep limited to remain responsive."
);

inline cvar<float> cvar_reflection_probe_near_z(
	cvar_flag::none,
	10.0f,
    "reflection_probe_near_z",
    "Near clipping plane of the view used to capture a reflection probes cubemap."
);

inline cvar<float> cvar_reflection_probe_far_z(
	cvar_flag::none,
	10'000.0f,
    "reflection_probe_far_z",
    "Far clipping plane of the view used to capture a reflection probes cubemap."
);

// ================================================================================================
//  Lighting
// ================================================================================================

inline cvar<int> cvar_lighting_max_lights(
	cvar_flag::none,
	10'000,
    "lighting_max_lights",
    "Maximum number of lights that can contribute to a single frame."
);

inline cvar<int> cvar_lighting_max_shadow_maps(
	cvar_flag::none,
	10'000,
    "lighting_max_shadow_maps",
    "Maximum number of shadow maps that can contribute to a single frame."
);

// ================================================================================================
//  Shadows
// ================================================================================================

inline cvar<int> cvar_shadows_max_cascade_updates_per_frame(
	cvar_flag::none,
	1,
    "shadows_max_cascade_updates_per_frame",
    "Maximum number of shadows cascades that can be updated per frame. The cascades chosen for update are always the ones that have been waiting the longest."
);

// ================================================================================================
//  Eye Adaption
// ================================================================================================

inline cvar<float> cvar_eye_adapation_min_luminance(
	cvar_flag::none,
	-8.0f,
    "eye_adapation_min_luminance",
    "Minimum luminance the eye adapation can handle."
);

inline cvar<float> cvar_eye_adapation_max_luminance(
	cvar_flag::none,
	3.5f,
    "eye_adapation_max_luminance",
    "Maximum luminance the eye adapation can handle."
);

inline cvar<float> cvar_eye_adapation_white_point(
	cvar_flag::none,
	3.0f,
    "eye_adapation_white_point",
    "Luminance value that is considered white."
);

inline cvar<float> cvar_eye_adapation_exposure_tau(
	cvar_flag::none,
	1.1f,
    "eye_adapation_exposure_tau",
    "Controls how fast the eye adaption adjusts to the current frames luminance."
);

// ================================================================================================
//  SSAO
// ================================================================================================

inline cvar<bool> cvar_ssao_enabled(
	cvar_flag::none,
	false,                          // Disabled for now, doesn't play nicely with multiple views.
    "ssao_enabled",
    "Turns SSAO on or off."
);

inline cvar<float> cvar_ssao_sample_radius(
	cvar_flag::none,
	3.0f,
    "ssao_sample_radius",
    "Determines over how large and area we sample texels to determine occlusion."
);

inline cvar<float> cvar_ssao_intensity_power(
	cvar_flag::none,
	100.0f,
    "ssao_intensity_power",
    "Determines to what power we raise the output AO, the higher the more contrast and stronger the SSAO effect is."
);

inline cvar<float> cvar_ssao_resolution_scale(
	cvar_flag::none,
	1.0f,
    "ssao_resolution_scale",
    "Determines the resolution scale ssao is run on, this can be adjusted to balance between performance and quality."
);

inline cvar<float> cvar_ssao_direct_light_effect(
	cvar_flag::none,
	0.0f,
    "ssao_direct_light_effect",
    "Determines how much effect the ssao has on direct lighting. In theory SSAO should only effect ambient lighting, but having a small amount added direct lighting avoids things looking flat."
);

// ================================================================================================
//  Raytracing
// ================================================================================================

inline cvar<bool> cvar_raytracing_enabled(
	cvar_flag::none,
	true,
    "raytracing_enabled",
    "Toggles on or off raytracing in its entirity."
);

}; // namespace ws
