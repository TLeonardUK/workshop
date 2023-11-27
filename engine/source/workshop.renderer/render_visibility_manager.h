// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"
#include "workshop.core/math/obb.h"
#include "workshop.core/math/frustum.h"
#include "workshop.core/containers/oct_tree.h"
#include "workshop.core/utils/traits.h"

#include <shared_mutex>
#include <unordered_map>
#include <bitset>

namespace ws {

class renderer;
class render_view;

// ================================================================================================
//  Generate flags that describe properties of an objects visibility.
// ================================================================================================
enum class render_visibility_flags
{
    none        = 0,

    // The object has a physical representation. If an object with this flag
    // moves within a view, the view is marked as having changed.
    physical    = 1,
};
DEFINE_ENUM_FLAGS(render_visibility_flags);

// ================================================================================================
//  Manages registration of bounding boxes and tracking of their visibility across all 
//  active views.
// ================================================================================================
class render_visibility_manager
{
public:

    // Opaque handle used to query and update object state.
    struct object_id
    {
    public:
        size_t get_hash() const
        {
            size_t h = 0;
            hash_combine(h, index);
            hash_combine(h, generation);
            return h;
        }

        size_t get_index() const
        {
            return index;
        }

        size_t get_generation() const
        {
            return generation;
        }

        bool operator==(const object_id& other) const
        {
            return index == other.index && generation == other.generation;
        }

        bool operator!=(const object_id& other) const
        {
            return !operator==(other);
        }

    private:
        friend class render_visibility_manager;

        size_t index;
        size_t generation;
    };

    // Opaque handle used to query and update view state.
    struct view_id 
    {
    public:
        size_t get_hash() const
        {
            size_t h = 0;
            hash_combine(h, index);
            hash_combine(h, generation);
            return h;
        }

        bool operator==(const view_id& other) const
        {
            return index == other.index && generation == other.generation;
        }

        bool operator!=(const view_id& other) const
        {
            return !operator==(other);
        }

    private:
        friend class render_visibility_manager;

        size_t index;
        size_t generation;
    };

public:

    render_visibility_manager(renderer& render);

    // Registers all the steps required to initialize the system.
    void register_init(init_list& list);

    // Updates the visibility of all objects in the scene with respect to all render views.
    void update_visibility();

public:

    // Objects

    // Registers a bounding box for visibility calculations.
    object_id register_object(const obb& bounds, render_visibility_flags flags);

    // Unregisters an object previously registered with register_object.
    void unregister_object(object_id id);

    // Updates the bounds of an object that is currently registered. Visibility
    // state will not be immediately updated, it will occur when update_visibility is next called.
    void update_object_bounds(object_id id, const obb& bounds);

    // Returns true if the object is visible inside the given view.
    bool is_object_visibile(view_id view_id, object_id object_id);

    // Allows manually setting an object as non-visible and overriding its normal visibility state.
    void set_object_manual_visibility(object_id id, bool visible);

    // Views

    // Registers a view that will determine visibility of objects.
    view_id register_view(const frustum& frustum, render_view* metadata = nullptr);
    
    // Unregisters a view previously allocated with register_view.
    void unregister_view(view_id id);

    // Returns true if the views frustum has changed or any objects with a physical flag inside 
    // its frustum have been moved.
    bool has_view_changed(view_id id);

    // Updates the frustum of the view.
    void update_object_frustum(view_id id, const frustum& bounds);

    // Sets if the view is active and visibility should be calculated for it. Otherwise its last
    // state persists.
    void set_view_active(view_id id, bool active);

    // Debug rendering.

    void draw_cell_bounds(bool draw_cell_bounds, bool draw_object_bounds);

private:

    inline static constexpr size_t k_object_states_growth_factor = 256;
    inline static constexpr size_t k_view_states_growth_factor = 16;
    inline static constexpr size_t k_max_tracked_views = 256;

    struct object_state
    {
        object_id id;
        bool used;

        obb bounds;
        render_visibility_flags flags;
        std::bitset<k_max_tracked_views> visibility;
        bool manual_visibility;

        oct_tree<object_id>::token oct_tree_entry;

        bool is_dirty;
    };

    struct view_state
    {
        view_id id;
        bool used;

        frustum bounds;

        bool is_dirty;
        bool has_changed;
        bool active;

        render_view* object;

        std::vector<object_id> visible_objects;
    };

private:
    inline static const vector3 k_octtree_extents = vector3(1'000'000.0f, 1'000'000.0f, 1'000'000.0f);
    inline static const size_t k_octtree_max_depth = 10;

    renderer& m_renderer;

    std::shared_mutex m_mutex;

    std::vector<object_id> m_dirty_objects;

    std::vector<object_state> m_objects;
    std::vector<view_state> m_views;

    std::vector<size_t> m_free_object_indices;
    std::vector<size_t> m_free_view_indices;

    oct_tree<object_id> m_oct_tree;

};

}; // namespace ws

// ================================================================================================
// Specialization of std::hash for the object/view id's
// ================================================================================================
template<>
struct std::hash<ws::render_visibility_manager::object_id>
{
    std::size_t operator()(const ws::render_visibility_manager::object_id& k) const
    {
        return k.get_hash();
    }
};

template<>
struct std::hash<ws::render_visibility_manager::view_id>
{
    std::size_t operator()(const ws::render_visibility_manager::view_id& k) const
    {
        return k.get_hash();
    }
};
