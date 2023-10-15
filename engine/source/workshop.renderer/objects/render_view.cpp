// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/objects/render_view.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_resource_cache.h"
#include "workshop.renderer/systems/render_system_debug.h"

namespace ws {

render_view::render_view(render_object_id id, renderer& renderer)
    : render_object(id, &renderer, render_visibility_flags::none)
{
    m_resource_cache = std::make_unique<render_resource_cache>(renderer);

    m_visibility_view_id = m_renderer->get_visibility_manager().register_view(get_frustum(), this);
}

render_view::~render_view()
{
    m_renderer->get_visibility_manager().unregister_view(m_visibility_view_id);
}

void render_view::bounds_modified()
{
    render_object::bounds_modified();

    m_renderer->get_visibility_manager().update_object_frustum(m_visibility_view_id, get_frustum());

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

void render_view::set_render_target(ri_texture_view view)
{
    m_render_target = view;

    update_view_info_param_block();
}

ri_texture_view render_view::get_render_target()
{
    return m_render_target;
}

bool render_view::has_render_target()
{
    return m_render_target.texture != nullptr;
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
        m_view_info_param_block = m_renderer->get_param_block_manager().create_param_block("view_info");
    }
    m_view_info_param_block->set("view_z_near", m_near_clip);
    m_view_info_param_block->set("view_z_far", m_far_clip);
    m_view_info_param_block->set("view_world_position", m_local_location);
    m_view_info_param_block->set("view_dimensions", vector2((float)m_viewport.width, (float)m_viewport.height));
    m_view_info_param_block->set("view_matrix", get_view_matrix());
    m_view_info_param_block->set("projection_matrix", get_projection_matrix());
    m_view_info_param_block->set("inverse_view_matrix", get_view_matrix().inverse());
    m_view_info_param_block->set("inverse_projection_matrix", get_projection_matrix().inverse());
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
    return m_renderer->get_visibility_manager().is_object_visibile(m_visibility_view_id, object->get_visibility_id());
}

bool render_view::has_view_changed()
{
    return m_renderer->get_visibility_manager().has_view_changed(m_visibility_view_id);
}

void render_view::set_should_render(bool value)
{
    m_should_render = value;
}

bool render_view::should_render()
{
    return m_should_render;
}

void render_view::set_active(bool value)
{
    m_active = value;

    m_renderer->get_visibility_manager().set_view_active(m_visibility_view_id, value);
}

bool render_view::get_active()
{
    return m_active;
}

render_visibility_manager::view_id render_view::get_visibility_view_id()
{
    return m_visibility_view_id;
}

}; // namespace ws
