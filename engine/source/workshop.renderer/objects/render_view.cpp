// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/objects/render_view.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_resource_cache.h"

namespace ws {

render_view::render_view(render_object_id id, render_scene_manager* scene_manager, renderer& renderer)
    : render_object(id, scene_manager, false)
    , m_renderer(renderer)
{
    m_resource_cache = std::make_unique<render_resource_cache>(renderer);
}

void render_view::bounds_modified()
{
    m_dirty = true;
}

void render_view::set_local_transform(const vector3& location, const quat& rotation, const vector3& scale)
{
    render_object::set_local_transform(location, rotation, scale);

    update_view_info_param_block();
}

void render_view::set_view_type(render_view_type type)
{
    m_view_type = type;

    update_view_info_param_block();
}

render_view_type render_view::get_view_type()
{
    return m_view_type;
}

void render_view::set_render_target(ri_texture* texture)
{
    m_render_target = texture;

    update_view_info_param_block();
}

ri_texture* render_view::get_render_target()
{
    return m_render_target;
}

void render_view::set_flags(render_view_flags flags)
{
    m_flags = flags;
}

render_view_flags render_view::get_flags()
{
    return m_flags;
}

bool render_view::has_flag(render_view_flags flags)
{
    return (static_cast<int>(m_flags) & static_cast<int>(flags)) != 0; 
}

void render_view::set_view_order(render_view_order order)
{
    m_render_view_order = order;
}

render_view_order render_view::get_view_order()
{
    return m_render_view_order;
}

void render_view::set_viewport(const recti& viewport)
{
    if (m_viewport == viewport)
    {
        return;
    }

    m_viewport = viewport;

    update_view_info_param_block();
    bounds_modified();
}

recti render_view::get_viewport()
{
    return m_viewport;
}

void render_view::set_clip(float near, float far)
{
    if (m_near_clip == near &&
        m_far_clip == far)
    {
        return;
    }

    m_near_clip = near;
    m_far_clip = far;

    update_view_info_param_block();
    bounds_modified();
}

void render_view::get_clip(float& near, float& far)
{
    near = m_near_clip;
    far = m_far_clip;
}

void render_view::set_fov(float fov)
{
    if (m_field_of_view == fov)
    {
        return;
    }

    m_field_of_view = fov;

    bounds_modified();
    update_view_info_param_block();
}

float render_view::get_fov()
{
    return m_field_of_view;
}

void render_view::set_aspect_ratio(float ratio)
{
    if (m_aspect_ratio == ratio)
    {
        return;
    }

    m_aspect_ratio = ratio;

    bounds_modified();
    update_view_info_param_block();
}

float render_view::get_aspect_ratio()
{
    return m_aspect_ratio;
}

void render_view::set_view_matrix(matrix4 value)
{
    if (m_custom_view_matrix == value)
    {
        return;
    }

    m_custom_view_matrix = value;

    bounds_modified();
}

matrix4 render_view::get_view_matrix()
{
    switch (m_view_type)
    {
    default:
    case render_view_type::perspective:
        {
            return matrix4::look_at(
                m_local_location,
                m_local_location + (vector3::forward * m_local_rotation),
                vector3::up);
        }
    case render_view_type::custom:
        {
            return m_custom_view_matrix;
        }
    }
}

void render_view::set_projection_matrix(matrix4 value)
{
    if (m_custom_projection_matrix == value)
    {
        return;
    }

    m_custom_projection_matrix = value;

    bounds_modified();
}

matrix4 render_view::get_projection_matrix()
{
    switch (m_view_type)
    {
    default:
    case render_view_type::perspective:
        {
            return matrix4::perspective(
                math::radians(m_field_of_view),
                m_aspect_ratio,
                m_near_clip,
                m_far_clip);
        }
    case render_view_type::custom:
        {
            return m_custom_projection_matrix;
        }
    }
}

ri_param_block* render_view::get_view_info_param_block()
{
    return m_view_info_param_block.get();
}

void render_view::update_view_info_param_block()
{
    if (!m_view_info_param_block)
    {
        m_view_info_param_block = m_renderer.get_param_block_manager().create_param_block("view_info");
    }
    m_view_info_param_block->set("view_matrix", get_view_matrix());
    m_view_info_param_block->set("projection_matrix", get_projection_matrix());
}

frustum render_view::get_frustum()
{
    return frustum(get_view_matrix() * get_projection_matrix());
}

frustum render_view::get_view_frustum()
{
    return frustum(get_projection_matrix());
}

render_resource_cache& render_view::get_resource_cache()
{
    return *m_resource_cache;
}

bool render_view::is_object_visible(render_object* object)
{
    if (visibility_index == render_view::k_always_visible_index)
    {
        return true;
    }
    return object->last_visible_frame[visibility_index] >= m_renderer.get_visibility_frame_index();
}

size_t render_view::get_last_change()
{
    return m_last_change;
}

void render_view::mark_dirty(size_t last_change)
{
    m_dirty = true;
    m_last_change = last_change;
}

bool render_view::is_dirty()
{
    return m_dirty;
}

void render_view::clear_dirty()
{
    m_dirty = false;
}

void render_view::set_should_render(bool value)
{
    m_should_render = value;
}

bool render_view::should_render()
{
    return m_should_render;
}

}; // namespace ws
