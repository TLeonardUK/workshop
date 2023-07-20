// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/aabb.h"
#include "workshop.core/math/sphere.h"
#include "workshop.core/math/frustum.h"
#include "workshop.core/async/async.h"
#include "workshop.core/utils/time.h"

#include <string>
#include <array>
#include <vector>
#include <functional>

namespace ws {

// ================================================================================================
//  Implementation of a spatial partitioning octtree 
// ================================================================================================
template <typename element_type>
class oct_tree
{
public:

    using intersect_function_t = std::function<bool(const aabb& bounds)>;

    struct cell;

    using cell_list_t = std::vector<cell*>;
    using element_list_t = std::vector<element_type>;
    using entry_id = size_t;

    struct entry
    {
        entry_id id;
        aabb bounds;
        element_type value;
        size_t last_changed;
    };

    struct cell
    {
        size_t depth;
        aabb bounds;
        cell* children[8];
        aabb children_bounds[8];
        std::vector<entry> elements;

        size_t child_elements = 0;
        cell* parent = nullptr;
        size_t parent_division_index = 0;

        size_t last_changed = 0;

        bool valid = false;

        cell(size_t depth_, const aabb& bounds_)
            : depth(depth_)
            , bounds(bounds_)
        {
            reset();
        }

        void reset()
        {
            memset(children, 0, sizeof(children));
            bounds.subdivide(children_bounds);
        }
    };

    struct token
    {
    public:
        bool is_valid()
        {
            return cell != nullptr;
        }

        void reset()
        {
            cell = nullptr;
        }

    private:
        friend class oct_tree<element_type>;

        entry_id id;
        cell* cell = nullptr;
    };


    struct intersect_result
    {
        std::vector<const entry*> entries;
        element_list_t elements;
        std::vector<const cell*> cells;
        size_t last_changed;
    };

public:
    oct_tree() = default;
    oct_tree(const vector3& extents, size_t max_depth);
    ~oct_tree();

    // Clears all elements from the tree.
    void clear();

    // Inserts an element that takes up the given bounds into the tree. The token
    // returned can be used to remove or modify the element.
    token insert(const aabb& bounds, element_type value);

    // Modifies the bounds of an existing entry.
    token modify(token token, const aabb& bounds, element_type value);

    // Removes an element using a token previous returned by insert.
    void remove(token token);

    // Gets all nodes in the tree.
    cell_list_t get_cells();

    // Finds all elements that overlap the given bounds.
    // 
    // If coarse is set then only the aabb of the cell containing the elements
    // is checked, otherwise the bounds of each individual element is checked.
    intersect_result intersect(const sphere& bounds, bool coarse, bool parallel = false) const;
    intersect_result intersect(const aabb& bounds, bool coarse, bool parallel = false) const;
    intersect_result intersect(const frustum& bounds, bool coarse, bool parallel = false) const;

private:

    // Gets the root node in the oct tree.
    cell& get_root();
    const cell& get_root() const;

    // Inserts an element into the given node or traverses down the tree addding
    // cells where needed until at a leaf node.
    token insert(const aabb& bounds, entry entry, cell& cell);

    // Gets all the leaf cells that intersect with the given 
    void get_cells(intersect_result& result, const cell& cell, intersect_function_t& insersect_function) const;

    // Gets all the elements with the cells that pass the insersection function.
    void get_elements(intersect_result& result, intersect_function_t& insersect_function, bool coarse, bool parallel) const;

    // Removes a cell from its parent and the nodes list.
    void remove_cell(cell& cell);

    // Marks a cell as changed and propogates it up the tree.
    void propogate_change(cell* cell);

private:

    size_t m_max_depth;
    entry_id m_next_entry_id = 1; 
    vector3 m_extents;
    std::vector<std::unique_ptr<cell>> m_cells;

    size_t m_change_counter = 0;

};

template <typename element_type>
inline oct_tree<element_type>::oct_tree(const vector3& extents, size_t max_depth)
    : m_max_depth(max_depth)
    , m_extents(extents)
{
    clear();
}

template <typename element_type>
inline oct_tree<element_type>::~oct_tree()
{
    clear();
}

template <typename element_type>
inline void oct_tree<element_type>::clear()
{
    m_cells.clear();
    m_cells.push_back(std::make_unique<cell>(0, aabb(-m_extents * 0.5f, m_extents * 0.5f)));

    get_root().valid = true;

    propogate_change(&get_root());
}

template <typename element_type>
inline oct_tree<element_type>::token oct_tree<element_type>::insert(const aabb& bounds, element_type value)
{
    entry entry;
    entry.id = m_next_entry_id++;
    entry.bounds = bounds;
    entry.value = value;
    entry.last_changed = m_change_counter++;

    return insert(bounds, entry, get_root());
}

template <typename element_type>
inline oct_tree<element_type>::token oct_tree<element_type>::modify(token token, const aabb& bounds, element_type value)
{
    remove(token);
    return insert(bounds, value);
}

template <typename element_type>
void oct_tree<element_type>::remove(token token)
{
    // This is very slow, but better than slow linear access for queries.
    bool removed = false;

    auto& entries = token.cell->elements;
    for (size_t i = 0; i < entries.size(); i++)
    {
        entry& entry = entries[i];
        if (entry.id == token.id)
        {
            entries[i] = entries[entries.size() - 1];
            entries.resize(entries.size() - 1);
            removed = true;
            break;
        }
    }

    if (!removed)
    {
        return;
    }

    // Mark cell as changed.
    propogate_change(token.cell);

    // Reduce child count up the tree, and remove any empty cells along the way.
    cell* iter = token.cell;
    while (iter != nullptr)
    {
        auto parent = iter->parent;

        if (iter->parent != nullptr && iter->child_elements == 0)
        {
            // If the lower cell was modified more recently than the parent then
            // propogate the new change value to its parent.
            parent->last_changed = std::max(parent->last_changed, iter->last_changed);

            remove_cell(*iter);
        }

        iter = parent;
    }
}

template <typename element_type>
void oct_tree<element_type>::propogate_change(cell* value)
{
    m_change_counter++;

    // Note: Don't propogate upwards anymore, we only care about individual cells
    // being dirty now, we propogate up only when cells are removed.
    /*
    cell* iter = value;
    while (iter != nullptr)
    {
        iter->last_changed = m_change_counter;
        iter = iter->parent;
    }*/
}

template <typename element_type>
void oct_tree<element_type>::remove_cell(cell& cell)
{
    // Remove from parents child cell list.
    cell.parent->children[cell.parent_division_index] = nullptr;
    cell.valid = false;

    // Remove from global list.
    auto iter = std::find_if(m_cells.begin(), m_cells.end(), [&cell](auto& item) {
        return item.get() == &cell;
    });
    if (iter != m_cells.end())
    {
        m_cells.erase(iter);
    }
}

template <typename element_type>
oct_tree<element_type>::cell_list_t oct_tree<element_type>::get_cells()
{
    cell_list_t result;
    result.reserve(m_cells.size());
    for (auto& entry : m_cells)
    {
        result.push_back(entry.get());
    }
    return result;
}

template <typename element_type>
oct_tree<element_type>::intersect_result oct_tree<element_type>::intersect(const sphere& bounds, bool coarse, bool parallel) const
{
    intersect_function_t func = [&bounds](const aabb& cell_bounds) -> bool {
        return bounds.intersects(cell_bounds);
    };

    intersect_result result;
    get_cells(result, get_root(), func);
    get_elements(result, func, coarse, parallel);

    return result;
}

template <typename element_type>
oct_tree<element_type>::intersect_result oct_tree<element_type>::intersect(const aabb& bounds, bool coarse, bool parallel) const
{
    intersect_function_t func = [&bounds](const aabb& cell_bounds) -> bool {
        return bounds.intersects(cell_bounds);
    };

    intersect_result result;
    get_cells(result, get_root(), func);
    get_elements(result, func, coarse, parallel);

    return result;
}

template <typename element_type>
oct_tree<element_type>::intersect_result oct_tree<element_type>::intersect(const frustum& bounds, bool coarse, bool parallel) const
{
    intersect_function_t func = [&bounds](const aabb& cell_bounds) -> bool {
        return bounds.intersects(cell_bounds) != frustum::intersection::outside;
    };

    intersect_result result;
    get_cells(result, get_root(), func);
    get_elements(result, func, coarse, parallel);

    return result;
}


template <typename element_type>
typename oct_tree<element_type>::cell& oct_tree<element_type>::get_root()
{
    return *m_cells[0];
}

template <typename element_type>
typename const oct_tree<element_type>::cell& oct_tree<element_type>::get_root() const
{
    return *m_cells[0];
}

template <typename element_type>
oct_tree<element_type>::token oct_tree<element_type>::insert(const aabb& bounds, entry entry, cell& insert_cell)
{
    size_t fits_in_child = 0;
    size_t fits_in_child_count = 0;

    // Can we fit into any of the nodes children?
    if (insert_cell.depth < m_max_depth)
    {
        for (size_t i = 0; i < 8; i++)
        {
            if (insert_cell.children_bounds[i].contains(bounds))
            {
                if (fits_in_child_count > 0)
                {
                    fits_in_child = -1;
                    fits_in_child_count = 0;
                    break;
                }

                fits_in_child = i;
                fits_in_child_count++;
            }
        }
    }

    // If we have an exclusive child we can fit in, insert into it.
    if (fits_in_child_count > 0)
    {
        if (insert_cell.children[fits_in_child] == nullptr)
        {
            std::unique_ptr<cell> new_cell = std::make_unique<cell>(insert_cell.depth + 1, insert_cell.children_bounds[fits_in_child]);
            new_cell->valid = true;
            new_cell->parent = &insert_cell;
            new_cell->parent_division_index = fits_in_child;

            insert_cell.children[fits_in_child] = new_cell.get();

            m_cells.push_back(std::move(new_cell));
        }

        insert_cell.child_elements++;

        return insert(bounds, entry, *insert_cell.children[fits_in_child]);
    }

    // Otherwise we are the largest node that can contain the area, so insert into us.
    else
    {
        insert_cell.elements.push_back(entry);
        insert_cell.child_elements++;
        
        propogate_change(&insert_cell);

        token ret;
        ret.id = entry.id;
        ret.cell = &insert_cell;

        return ret;
    }
}

template <typename element_type>
void oct_tree<element_type>::get_cells(intersect_result& result, const cell& cell, intersect_function_t& insersect_function) const
{
    if (cell.elements.size() > 0)
    {
        result.cells.push_back(&cell);
    }

    for (size_t i = 0; i < 8; i++)
    {
        if (cell.children[i] && insersect_function(cell.children_bounds[i]))
        {
            get_cells(result, *cell.children[i], insersect_function);
        }
    }
}

template <typename element_type>
void oct_tree<element_type>::get_elements(intersect_result& result, intersect_function_t& insersect_function, bool coarse, bool parallel) const
{
    size_t max_results = 0;

    for (auto& cell : result.cells)
    {
        max_results += cell->elements.size();
    }

    result.elements.resize(max_results);
    result.entries.resize(max_results);

    std::atomic_size_t total_results = 0;

    auto callback = [&result, &total_results, coarse, &insersect_function](size_t i) mutable {

        const cell& cell = *result.cells[i];
        for (size_t j = 0; j < cell.elements.size(); j++)
        {
            const entry& entry = cell.elements[j];
            if (coarse || insersect_function(entry.bounds))
            {
                size_t result_index = total_results.fetch_add(1);
                result.entries[result_index] = &entry;
                result.elements[result_index] = entry.value;
            }
        }
    };

    if (parallel)
    {
        parallel_for("octtree gather", task_queue::standard, result.cells.size(), callback, true);
    }
    else
    {
        for (size_t i = 0; i < result.cells.size(); i++)
        {
            callback(i);
        }
    }

    result.elements.resize(total_results);
    result.entries.resize(total_results);

    // Grab the most recent change while we are at this.
    result.last_changed = 0;
    if (coarse)
    {
        for (auto& cell : result.cells)
        {
            result.last_changed = std::max(result.last_changed, cell->last_changed);
        }
    }
    else
    {
        for (auto& entry : result.entries)
        {
            result.last_changed = std::max(result.last_changed, entry->last_changed);
        }
    }
}

}; // namespace workshop
