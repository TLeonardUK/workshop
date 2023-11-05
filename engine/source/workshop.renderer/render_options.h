// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

namespace ws {

// ================================================================================================
//  This class contains all configurable options in the rendering pipeline. This can be changed
// 	dynamically by using the render_command_queue::set_render_options function.
// ================================================================================================
class render_options
{
public:

	// ================================================================================================
	//  Textures
	// ================================================================================================

	// How many mips to drop of a texture as its loaded. This can be used to quickly strim down the 
	// maximum memory being used. In general texture streaming/etc should be used rather than this.
	size_t textures_dropped_mips = 0;

	// ================================================================================================
	//  Light Probes
	// ================================================================================================

	// How many rays to cast per probe to calculate diffus lighting.
	size_t light_probe_ray_count = 196;

	// How many probes can be regenerated per frame. Each regeneration can cost as much as an entire
	// scene render, so keep limited to remain responsive.
	size_t light_probe_max_regenerations_per_frame = 16;

	// Far clipping plane of the view used to capture a light probes cubemap.
	float light_probe_far_z = 100'000.0f;

	// Light probes are prioritized for rendering based on how close they are to a "normal" view.
	// The prioritization list is only updated whenever the view moves by this amount.
	float light_probe_queue_update_distance = 1'000.0f;

    // Size of the irradiance map for probe. Larger means more memory usage, but gives more
    // accurate avoidance of light leaking.
    size_t light_probe_irradiance_map_size = 4;

    // Size of the occlusion map for probe. Larger means more memory usage, but gives more
    // accurate avoidance of light leaking.
    size_t light_probe_occlusion_map_size = 14;

    // Exponent used for depth testing. High values react quickly to depth discontinuties but may cause banding.
    float light_probe_distance_exponent = 50.0f;

	// ================================================================================================
	//  Reflection Probes
	// ================================================================================================

	// Size of the cubemap that is captured for reflection
	size_t reflection_probe_cubemap_size = 512;

	// Number of mips each reflection probe generates at varying levels of roughness.
	size_t reflection_probe_cubemap_mip_count = 10;

	// How many probes can be regenerated per frame. Each regeneration can cost as much as an entire
	// scene render, so keep limited to remain responsive.
	size_t reflection_probe_max_regenerations_per_frame = 1;

	// Near clipping plane of the view used to capture a reflection probes cubemap.
	float reflection_probe_near_z = 10.0f;

	// Far clipping plane of the view used to capture a reflection probes cubemap.
	float reflection_probe_far_z = 10'000.0f;

	// ================================================================================================
	//  Lighting
	// ================================================================================================

	// Maximum number of lights that can contribute to a single frame.
	size_t lighting_max_lights = 10'000;

	// Maximum number of shadow maps that can contribute to a single frame.
	size_t lighting_max_shadow_maps = 10'000;

	// ================================================================================================
	//  Shadows
	// ================================================================================================

	// Maximum number of shadows cascades that can be updated per frame. The cascades chosen for update
	// are always the ones that have been waiting the longest.
    //
    // 3 for main view plus (6*3) for probe captures, bleh.
    // we need to figure out a better way of handling this for probe captures (one face rendered per frame?)
	size_t shadows_max_cascade_updates_per_frame = 1; 

	// ================================================================================================
	//  Eye Adaption
	// ================================================================================================
	
	// Minimum luminance the eye adapation can handle.
	float eye_adapation_min_luminance = -8.0f;

	// Maximum luminance the eye adapation can handle.
	float eye_adapation_max_luminance = 3.5f;

	// Luminance value that is considered white.
	float eye_adapation_white_point = 3.0f;

	// Controls how fast the eye adaption adjusts to the current frames luminance.
	float eye_adapation_exposure_tau = 1.1f;

	// ================================================================================================
	//  SSAO
	// ================================================================================================

	// Determines over how large and area we sample texels to determine occlusion.
	float ssao_sample_radius = 15.0f;

	// Determines to what power we raise the output AO, the higher the more contrast 
	// and stronger the SSAO effect is.
	float ssao_intensity_power = 100.0f;

	// Determines the resolution scale ssao is run on, this can be adjusted to balance between performance and quality.
	float ssao_resolution_scale = 0.5f;

	// Determines how much effect the ssao has on direct lighting. In theory SSAO should only effect
	// ambient lighting, but having a small amount added direct lighting avoids things looking flat.
	float ssao_direct_light_effect = 0.5f;

    // ================================================================================================
    //  Raytracing
    // ================================================================================================

    // Toggles on or off raytracing in its entirity.
    bool raytracing_enabled = true;

};

}; // namespace ws
