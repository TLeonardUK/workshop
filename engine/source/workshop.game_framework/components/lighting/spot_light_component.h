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
//  Represents a spot light in the world.
// ================================================================================================
class spot_light_component : public light_component
{
public:

	// The inner radius of the spotlight. The intensity is attenuated linearly between the radii.
	// The range is in radians between [0, pi]
	float inner_radius = 0.0f;
	
	// The outer radius of the spotlight. The intensity is attenuated linearly between the radii.
	// The range is in radians between [0, pi]
	float outer_radius = 0.2f;

protected:

	friend class spot_light_system;

};

}; // namespace ws
