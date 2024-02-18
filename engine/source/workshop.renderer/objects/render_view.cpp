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

    m_visibility_view_id = m_renderer->get_visibility_manager().register_view(get_frustum(), m_world_id, this);
}

render_view::~render_view()
{
    m_renderer->get_visibility_manager().unregister_view(m_visibility_view_id);
}

void render_view::bounds_modified()
{
    render_object::bounds_modified();

    m_renderer->get_visibility_manager().update_object_frustum(m_visibility_view_id, m_world_id, get_frustum());

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
    update_render_target_flags();
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

    update_render_target_flags();
    update_visibility_flags();
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

void render_view::set_visualization_mode(visualization_mode mode)
{
    m_visualization_mode = mode;
}

visualization_mode render_view::get_visualization_mode()
{
    return m_visualization_mode;
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
    case render_view_type::ortographic:
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


void render_view::set_orthographic_rect(rect value)
{
    m_ortho_rect = value;
}

rect render_view::get_orthographic_rect()
{
    return m_ortho_rect;
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
    case render_view_type::ortographic:
        {
            return matrix4::orthographic(
                m_ortho_rect.x,
                m_ortho_rect.x + m_ortho_rect.width,
                m_ortho_rect.y + m_ortho_rect.height,
                m_ortho_rect.y,
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
    m_view_info_param_block->set("view_z_near"_sh, m_near_clip);
    m_view_info_param_block->set("view_z_far"_sh, m_far_clip);
    m_view_info_param_block->set("view_world_position"_sh, m_local_location);
    m_view_info_param_block->set("view_dimensions"_sh, vector2((float)m_viewport.width, (float)m_viewport.height));
    m_view_info_param_block->set("view_matrix"_sh, get_view_matrix());
    m_view_info_param_block->set("projection_matrix"_sh, get_projection_matrix());
    m_view_info_param_block->set("inverse_view_matrix"_sh, get_view_matrix().inverse());
    m_view_info_param_block->set("inverse_projection_matrix"_sh, get_projection_matrix().inverse());
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
    // If in the editor and not marked to render in the editor mode, then ignore.
    if (m_renderer->in_editor() && !has_flag(render_view_flags::render_in_editor_mode))
    {
        return false;
    }
    return m_should_render;
}

void render_view::set_will_render(bool value)
{
    m_will_render = value;
}

bool render_view::will_render()
{
    return m_will_render;
}

void render_view::force_render()
{
    m_force_render = true;
}

bool render_view::consume_force_render()
{
    bool ret = m_force_render;
    m_force_render = false;
    return ret;
}

void render_view::set_active(bool value)
{
    m_active = value;

    update_visibility_flags();
}

bool render_view::get_active()
{
    return m_active;
}

void render_view::update_visibility_flags()
{
    bool active = m_active;
    if (has_flag(render_view_flags::freeze_rendering))
    {
        active = false;
    }

    m_renderer->get_visibility_manager().set_view_active(m_visibility_view_id, active);
}

render_visibility_manager::view_id render_view::get_visibility_view_id()
{
    return m_visibility_view_id;
}

void render_view::set_readback_pixmap(pixmap* output)
{
    m_readback_pixmap = output;

    // If no render target is specified create a new readback RT.
    if (!m_readback_rt && output)
    {
        db_assert(output->get_format() == pixmap_format::R8G8B8A8_SRGB);

        // Create render target to write the view to.
        ri_texture::create_params create_params;
        create_params.dimensions = ri_texture_dimension::texture_2d;
        create_params.width = output->get_width();
        create_params.height = output->get_height();
        create_params.mip_levels = 1;
        create_params.is_render_target = true;
        create_params.format = ri_texture_format::R8G8B8A8_SRGB;

        m_readback_rt = m_renderer->get_render_interface().create_texture(create_params, "view readback render target");;

        m_render_target.mip = 0;
        m_render_target.slice = 0;
        m_render_target.texture = m_readback_rt.get();

        // Create a readback buffer to copy the RT into.
        ri_buffer::create_params buffer_create_params;
        buffer_create_params.element_count = 1;
        buffer_create_params.element_size = m_readback_rt->get_pitch() * m_readback_rt->get_height() * 4;
        buffer_create_params.usage = ri_buffer_usage::readback;

        m_readback_buffer = m_renderer->get_render_interface().create_buffer(buffer_create_params, "view readback buffer");
    
        update_render_target_flags();
    }
}

pixmap* render_view::get_readback_pixmap()
{
    return m_readback_pixmap;
}

ri_buffer* render_view::get_readback_buffer()
{
    return m_readback_buffer.get();
}

void render_view::update_render_target_flags()
{
    // If we are drawing to an RT, we don't want to add all the capture flags.
    if (m_render_target.texture)
    {
        m_flags = m_flags | render_view_flags::capture;
    }
    else
    {
        m_flags = m_flags & ~render_view_flags::capture;
    }
}

}; // namespace ws
