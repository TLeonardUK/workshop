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
//  Represets a cubemap captured at a given point in the scene which is used to contribute towards
//  the indirect specular component of our lighting.
// ================================================================================================
class render_reflection_probe : public render_object
{
public:
    render_reflection_probe(render_object_id id, renderer& renderer);
    virtual ~render_reflection_probe();

    // Overrides the default bounds to return the obb of the model bounds.
    virtual obb get_bounds() override;

    // Called when the bounds of an object is modified.
    virtual void bounds_modified();

    // Gets the cubemap representing as the reflection probe.
    ri_texture& get_texture();

    // Gets the param block describing this probe for rendering it as part of the lighting pass.
    ri_param_block& get_param_block();

    // If reflection probe needs to be regenerated.
    bool is_dirty();

    // If reflection probe can be used.
    bool is_ready();

    // Called when probe has been regenerated, clears dirty flags and marks it ready for use.
    void mark_regenerated();

private:    
    std::unique_ptr<ri_texture> m_texture;
    std::unique_ptr<ri_param_block> m_param_block;

    bool m_dirty = true;
    bool m_ready = false;

};

}; // namespace ws
