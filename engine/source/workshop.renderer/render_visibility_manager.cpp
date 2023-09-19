// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_visibility_manager.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/systems/render_system_debug.h"

namespace ws {
    
render_visibility_manager::render_visibility_manager(renderer& render)
    : m_renderer(render)
    , m_oct_tree(k_octtree_extents, k_octtree_max_depth)
{
}

void render_visibility_manager::register_init(init_list& list)
{
}

render_visibility_manager::object_id render_visibility_manager::register_object(const obb& bounds, render_visibility_flags flags)
{
    std::unique_lock lock(m_mutex);

    if (m_free_object_indices.empty())
    {
        m_objects.reserve(m_free_object_indices.size() + k_object_states_growth_factor);

        for (size_t i = 0; i < k_object_states_growth_factor; i++)
        {
            object_state& state = m_objects.emplace_back();
            state.id.index = m_objects.size() - 1;
            state.id.generation = 0;

            m_free_object_indices.push_back(state.id.index);
        }
    }

    object_id id;
    id.index = m_free_object_indices.back();
    m_free_object_indices.pop_back();
    
    object_state& state = m_objects[id.index];
    state.id.generation++;
    id.generation = state.id.generation;
    state.used = true;
    state.bounds = bounds;
    state.flags = flags;
    state.visibility.reset();
    state.is_dirty = true;
    state.manual_visibility = true;
    state.oct_tree_entry = m_oct_tree.insert(bounds.get_aligned_bounds(), id);

    m_dirty_objects.push_back(id);

    return id;
}

void render_visibility_manager::unregister_object(object_id id)
{
    std::unique_lock lock(m_mutex);

    object_state& state = m_objects[id.index];
    if (state.id.generation == id.generation)
    {
        state.used = false;
        state.id.generation++;

        m_oct_tree.remove(state.oct_tree_entry);

        m_free_object_indices.push_back(state.id.index);
    }
}

void render_visibility_manager::update_object_bounds(object_id id, const obb& bounds)
{
    std::unique_lock lock(m_mutex);

    object_state& state = m_objects[id.index];
    if (state.id.generation == id.generation)
    {
        state.bounds = bounds;
        state.oct_tree_entry = m_oct_tree.modify(state.oct_tree_entry, bounds.get_aligned_bounds(), state.id);

        if (!state.is_dirty)
        {
            state.is_dirty = true;
            m_dirty_objects.push_back(id);
        }
    }
}

bool render_visibility_manager::is_object_visibile(view_id view_id, object_id object_id)
{
    std::shared_lock lock(m_mutex);

    object_state& object = m_objects[object_id.index];
    view_state& view = m_views[view_id.index];
    if (object.id.generation == object_id.generation && view.id.generation == view_id.generation)
    {
        if (view.id.index >= k_max_tracked_views)
        {
            return true;
        }
        else
        {
            return object.visibility[view.id.index];
        }
    }

    return false;
}

void render_visibility_manager::set_object_manual_visibility(object_id id, bool visible)
{
    std::unique_lock lock(m_mutex);

    object_state& state = m_objects[id.index];
    if (state.id.generation == id.generation && state.manual_visibility != visible)
    {
        state.manual_visibility = visible;
        db_log(core, "set_object_manual_visibility: id=%zi visible=%i", id.index, visible);

        if (!state.is_dirty)
        {
            state.is_dirty = true;
            m_dirty_objects.push_back(id);
        }
    }
}

render_visibility_manager::view_id render_visibility_manager::register_view(const frustum& frustum, render_view* metadata)
{
    std::unique_lock lock(m_mutex);

    if (m_free_view_indices.empty())
    {
        m_views.reserve(m_free_view_indices.size() + k_view_states_growth_factor);

        for (size_t i = 0; i < k_view_states_growth_factor; i++)
        {
            view_state& state = m_views.emplace_back();
            state.id.index = m_views.size() - 1;
            state.id.generation = 0;

            m_free_view_indices.push_back(state.id.index);
        }
    }

    view_id id;
    id.index = m_free_view_indices.back();
    m_free_view_indices.pop_back();

    view_state& state = m_views[id.index];
    state.id.generation++;
    state.is_dirty = true;
    state.used = true;
    state.active = true;
    state.object = metadata;

    id.generation = state.id.generation;

    return id;
}

void render_visibility_manager::unregister_view(view_id id)
{
    std::unique_lock lock(m_mutex);

    view_state& state = m_views[id.index];
    if (state.id.generation == id.generation)
    {
        state.used = false;
        state.id.generation++;

        m_free_view_indices.push_back(state.id.index);
    }
}

bool render_visibility_manager::has_view_changed(view_id id)
{
    std::shared_lock lock(m_mutex);

    view_state& state = m_views[id.index];
    if (state.id.generation == id.generation)
    {
        return state.has_changed;
    }

    return true;
}

void render_visibility_manager::set_view_active(view_id id, bool active)
{
    std::shared_lock lock(m_mutex);

    view_state& state = m_views[id.index];
    if (state.id.generation == id.generation)
    {
        state.active = active;
    }
}

void render_visibility_manager::update_object_frustum(view_id id, const frustum& bounds)
{
    std::unique_lock lock(m_mutex);

    view_state& state = m_views[id.index];
    if (state.id.generation == id.generation)
    {
        state.bounds = bounds;
        state.is_dirty = true;
    }
}

void render_visibility_manager::draw_cell_bounds(bool draw_cell_bounds, bool draw_object_bounds)
{
    std::shared_lock lock(m_mutex);

    render_system_debug* debug_system = m_renderer.get_system<render_system_debug>();

    auto cells = m_oct_tree.get_cells();
    for (auto& cell : cells)
    {
        if (draw_cell_bounds)
        {
            debug_system->add_aabb(cell->bounds, color::green);
        }

        for (auto& entry : cell->elements)
        {
            if (draw_object_bounds)
            {
                debug_system->add_aabb(entry.bounds, color::blue);
            }
        }
    }
}

#pragma optimize("", off)

void render_visibility_manager::update_visibility()
{
    std::scoped_lock lock(m_mutex);

    profile_marker(profile_colors::render, "update visibility");

    // Basic explanation of our visibility algorithm:

    // For each view:
    //      - If view is marked as dirty, mark view as changed
    //      - Get all visible objects
    //      - Compare against existing visibility state.
    //      -   -   if new object: change its visibility, and mark view as changed
    //      -   -   if removed object: change its visibility, and mark view as changed
    //      -   -   if no changes: if object is marked dirty, mark view as changed

    // For all dirty objects:
    //      - clear dirty flag

    // Grab all views to update.
    std::vector<size_t> view_indices;
    for (size_t i = 0; i < m_views.size(); i++)
    {
        view_state& state = m_views[i];
        state.has_changed = false;

        if (state.used && state.active)
        {
            view_indices.push_back(i);
        }
    }

    // Update visibility for each view.
    parallel_for("update views", task_queue::standard, view_indices.size(), [this, &view_indices](size_t view_list_index) {

        profile_marker(profile_colors::render, "update view visibility");

        size_t view_index = view_indices[view_list_index];

        view_state& state = m_views[view_index];
        if (!state.used || !state.active)
        {
            return;
        }
        
        if (state.is_dirty)
        {
            state.is_dirty = false;
            state.has_changed = true;
        }

        decltype(m_oct_tree)::intersect_result visible_objects = m_oct_tree.intersect(state.bounds, false, false);

        std::unordered_set<object_id> visible_object_ids;

        // Remove any objects that have been manually made invisible.
        for (auto iter = visible_objects.elements.begin(); iter != visible_objects.elements.end(); /* empty */)
        {
            object_id id = *iter;
            object_state& obj_state = m_objects[id.index];
            if (!obj_state.manual_visibility)
            {
                iter = visible_objects.elements.erase(iter);
            }
            else
            {
                iter++;
            }
        }

        // Go through visible objects and update states based on if they have entered/left/remained in the view.
        for (object_id object_id : visible_objects.elements)
        {
            object_state& obj_state = m_objects[object_id.index];

            // Object newly entering view.
            if (!obj_state.visibility[view_index])
            {
                obj_state.visibility[view_index] = true;
                
                // View is not marked as changed if the object has not physical representation.
                if (static_cast<int>(obj_state.flags & render_visibility_flags::physical) != 0)
                {
                    state.has_changed = true;
                }
            }
            // Existing object that was already in view.
            else
            {
                // If object itself is dirty then its moved inside the view so mark view as changed.
                if (obj_state.is_dirty && static_cast<int>(obj_state.flags & render_visibility_flags::physical) != 0)
                {
                    state.has_changed = true;
                }
            }

            visible_object_ids.insert(object_id);
        }

        // Go through list of objects marked as in the view, if any have been removed, update their 
        // visibility and mark view as changed.
        for (object_id existing_object_id : state.visible_objects)
        {
            object_state& obj_state = m_objects[existing_object_id.index];

            // View is not marked as changed if the object has not physical representation.
            if (static_cast<int>(obj_state.flags & render_visibility_flags::physical) == 0)
            {
                continue;
            }

            // Ensure object hasn't been removed/recycled before we update anything.
            if (obj_state.id.generation == existing_object_id.generation)
            {
                // See if object has been removed.
                if (visible_object_ids.find(existing_object_id) == visible_object_ids.end())
                {
                    obj_state.visibility[view_index] = false;
                    state.has_changed = true;
                }
            }
        }

        // Store visible objects to compare against next frame.
        state.visible_objects = visible_objects.elements;
    
    });

    // Clear dirty flag from all dirty objects.
    for (object_id object_id : m_dirty_objects)
    {
        object_state& obj_state = m_objects[object_id.index];
        obj_state.is_dirty = false;
    }
    m_dirty_objects.clear();
}

}; // namespace ws
