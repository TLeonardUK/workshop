// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_graph.h"

namespace ws {

render_graph::node_id render_graph::add_node(std::unique_ptr<render_pass>&& pass, const std::vector<node_id>& dependencies)
{
    size_t index = m_nodes.size();
    m_nodes.resize(m_nodes.size() + 1);

    node& new_node = m_nodes[index];
    new_node.pass = std::move(pass);
    new_node.dependencies = dependencies;

    return index;
}

void render_graph::flatten()
{
    m_flattened.resize(m_nodes.size());

    for (size_t i = 0; i < m_nodes.size(); i++)
    {
        m_flattened[i] = &m_nodes[i];
    }
}

std::vector<render_graph::node*>& render_graph::get_flattened_nodes()
{
    return m_flattened;
}

}; // namespace ws
