// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_types.h"
#include "workshop.core/drawing/color.h"

#include <span>

namespace ws {

class ri_texture;

// ================================================================================================
//  A staging buffer is an intermediate buffer that data is copied to, to make it 
//  accessible to the gpu.
// 
//  Staging buffers are usually used in situations where data needs to be uploaded to a resource
//  such as a texture or buffer.
// 
//  Moving data to a staging buffer can take a fairly long amount of time depending on the size
//  as it has to move across the PCI bus to the GPU. As such this class works asyncronously, and
//  cannot be used for any uploads until is_staged returns true, which will occur some amount of 
//  milliseconds after its created.
// ================================================================================================
class ri_staging_buffer
{
public:

    struct create_params
    {
        // The texture that the buffer will eventually be copied to after staged.
        ri_texture* destination;

        // The destination mip in the texture that the buffer.
        size_t mip_index;

        // The destination array index in the texture that the buffer.
        size_t array_index;

    };
public:
    virtual ~ri_staging_buffer() {}

    // Returns true when the buffer has finished being staged and ready to be used.
    virtual bool is_staged() = 0;

    // Waits until the buffer has finished being staged. Avoid use as it will 
    // cause stalls.
    virtual void wait() = 0;
    
};

}; // namespace ws
