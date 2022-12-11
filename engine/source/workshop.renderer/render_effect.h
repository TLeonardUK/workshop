// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"
#include "workshop.render_interface/ri_pipeline.h"

namespace ws {

class ri_pipeline;

// ================================================================================================
//  Represents an effect that can be used to perform a given render pass.
// 
//  An effect is group of techniques, each technique encapsulates an individual pipeline state 
//  object. The technique used to render an effect is selected based on variation parameters 
//  provided when the effect is chosen.
// 
//  The multiple-technique system is used to allow more easy variation of shaders. For example
//  you could have an effect called "generate_vrs_texture" that creates output a target used elsewhere.
//  On one piece of hardware it may be more effect to calculate values in 8 pixel tiles, on others
//  it may be more effect to do it in 16 pixel tiles. To more easily support this you can have two 
//  actual techniques (and thus compiled pipelines) for each of the tile size variations. When 
//  rendering you just reference "generate_vrs_technique", and then the render disambiguates
//  the actual technique to use by which techniques accepted certain variation parameters, in
//  this example we could use a renderer-provided "platform" variation to use the 8 pixel tile
//  size technique on one platform and the 16 pixel tile technique on the other.
// ================================================================================================
class render_effect
{
public:

    struct variation_parameter
    {
        std::string name;

        // Values accepted by this variation parameter. If no parameter is defined here its assumed
        // that the technique allows all values for it.
        std::vector<std::string> values;
    };

    struct technique
    {
        std::string name;

        // List of variation parameters, and their accepted values, as used
        // when selecting an appropriate technique to use.
        std::vector<variation_parameter> variation_parameters;

        // The actual pipeline that is used to render this effect. Ownership over this pipeline
        // is held by the creator of the render_effect (normally a shader asset). If 
        std::unique_ptr<ri_pipeline> pipeline;
    };

    // Name of the effect, used by render 
    std::string name;

    // List of techniques the effect contains.
    std::vector<technique> techniques;


};

}; // namespace ws
