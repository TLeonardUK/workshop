// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_graph.h"

namespace ws {

render_graph::node_id render_graph::add_node(std::unique_ptr<render_pass>&& pass, render_view_flags flags)
{
    size_t index = m_nodes.size();
    m_nodes.resize(m_nodes.size() + 1);

    node& new_node = m_nodes[index];
    new_node.pass = std::move(pass);
    new_node.required_flags = flags;
    new_node.enabled = true;

    return index;
}

void render_graph::set_node_enabled(node_id id, bool enabled)
{
    size_t index = static_cast<size_t>(id);
    m_nodes[id].enabled = enabled;
}

void render_graph::get_active(std::vector<node*>& nodes, render_view_flags flags)
{
    nodes.reserve(m_nodes.size());

    for (size_t i = 0; i < m_nodes.size(); i++)
    {
        if (m_nodes[i].enabled)
        {
            if ((static_cast<int>(m_nodes[i].required_flags) & static_cast<int>(flags)) == static_cast<int>(m_nodes[i].required_flags))
            {
                nodes.push_back(&m_nodes[i]);
            }
        }
    }
}

void render_graph::get_nodes(std::vector<node*>& nodes)
{
    nodes.reserve(m_nodes.size());

    for (size_t i = 0; i < m_nodes.size(); i++)
    {
        nodes.push_back(&m_nodes[i]);
    }
}

}; // namespace ws
