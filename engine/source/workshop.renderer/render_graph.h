// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"

#include "workshop.renderer/render_pass.h"

namespace ws {

class ri_command_list;

// ================================================================================================
//  The render graph is representation of all the render passes that need to be
//  executed to draw the scene.
// ================================================================================================
class render_graph
{
public:
    using node_id = size_t;
    constexpr static inline size_t k_invalid_node_id = std::numeric_limits<size_t>::max();

    struct node
    {
        std::unique_ptr<render_pass> pass;
        bool enabled;
    };

    // Adds a new node to the graph.    
    // Command lists for each node are generated in parallel. Dependencies are used
    // to ensure ordering between node generation.
    node_id add_node(std::unique_ptr<render_pass>&& pass);

    // Toggles a node on/off.
    void set_node_enabled(node_id id, bool enabled);

    // Gets the current list of render graph nodes that are active and 
    // should currentl participate in rendering.
    void get_active(std::vector<node*>& nodes);

    // Gets all nodes.
    void get_nodes(std::vector<node*>& nodes);

private:
    std::vector<node> m_nodes;

};

}; // namespace ws
