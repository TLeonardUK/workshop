// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"

#include "workshop.renderer/render_pass.h"

namespace ws {

class render_command_list;

// ================================================================================================
//  The render graph is hierarchical representation of all the render passes that need to be
//  executed to render the scene.
// 
//  The graph is normally built once and used every frame.
// ================================================================================================
class render_graph
{
public:
    using node_id = size_t;
    constexpr static inline size_t k_invalid_node_id = std::numeric_limits<size_t>::max();

    struct node
    {
        std::unique_ptr<render_pass> pass;
        std::vector<node_id> dependencies;
    };

    // Adds a new node to the graph.    
    // Command lists for each node are generated in parallel. Dependencies are used
    // to ensure ordering between node generation.
    node_id add_node(std::unique_ptr<render_pass>&& pass, const std::vector<node_id>& dependencies);

    void flatten();

    std::vector<node*>& get_flattened_nodes();

private:
    std::vector<node> m_nodes;
    std::vector<node*> m_flattened;

};

// ================================================================================================
//  Contains the generated state of a render graph node.
// ================================================================================================
class render_graph_node_generated_state
{
public:

    std::vector<render_command_list*> graphics_command_lists;

};

}; // namespace ws
