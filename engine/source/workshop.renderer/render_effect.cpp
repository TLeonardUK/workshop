// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_effect.h"

namespace ws {

void render_effect::swap(render_effect* other)
{
    std::swap(name, other->name);
    
    // Don't do a straight swap of the vector here, we want to keep the pointers the same so swap the 
    // contents of the techniques instead.
    for (auto& technique : techniques)
    {
        for (auto& other_technique : other->techniques)
        {
            if (technique->name == other_technique->name)
            {
                std::swap(technique->variation_parameters, other_technique->variation_parameters);
                std::swap(technique->pipeline, other_technique->pipeline);
            }
        }
    }
}

}; // namespace ws
