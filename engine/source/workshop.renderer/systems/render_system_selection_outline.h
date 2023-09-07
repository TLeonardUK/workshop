// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/render_system.h"
#include "workshop.renderer/render_pass.h"

#include "workshop.render_interface/ri_texture.h"
#include "workshop.render_interface/ri_buffer.h"

#include "workshop.core/math/vector2.h"
#include "workshop.core/math/vector4.h"
#include "workshop.core/math/rect.h"
#include "workshop.core/math/frustum.h"
#include "workshop.core/math/sphere.h"
#include "workshop.core/math/hemisphere.h"
#include "workshop.core/math/cylinder.h"

namespace ws {

class renderer;
class render_pass_callback;
class render_view;

// ================================================================================================
//  Renders an edge outline on fragments marked with the selected flag.
// ================================================================================================
class render_system_selection_outline
    : public render_system
{
public:
    render_system_selection_outline(renderer& render);

    virtual void register_init(init_list& list) override;
    virtual void step(const render_world_state& state) override;
    virtual void build_graph(render_graph& graph, const render_world_state& state, render_view& view) override;

};

}; // namespace ws
