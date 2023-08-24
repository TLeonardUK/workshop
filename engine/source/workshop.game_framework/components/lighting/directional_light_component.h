// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/component.h"
#include "workshop.game_framework/components/lighting/light_component.h"

#include "workshop.core/math/quat.h"
#include "workshop.core/math/vector3.h"

#include "workshop.renderer/render_object.h"

namespace ws {

// ================================================================================================
//  Represents a directional light in the world.
// ================================================================================================
class directional_light_component : public light_component
{
public:

	// Number of cacades the directional light has. Later cascades cover larger and larger areas. 
	// Having multiple allows for good shadow detail and large distances than having a single one
	// that covers the same area.
	size_t shadow_cascades = 3;

	// Determines how the cascades are split across the viewing frustum.
	// The lower the exponent the closer to linear the split becomes.
	float shadow_cascade_exponent = 0.75f;

	// Sets the fraction of a cascade that is used to blend between it and the next cascade. 
	// Provides a gradual transition between the cascades.
	float shadow_cascade_blend = 0.1f;

protected:

	friend class directional_light_system;

};

}; // namespace ws
