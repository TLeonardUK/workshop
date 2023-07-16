// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/objects/render_view.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_resource_cache.h"

namespace ws {

render_view::render_view(render_scene_manager* scene_manager, renderer& renderer)
    : render_object(scene_manager, false)
    , m_renderer(renderer)
{
    m_resource_cache = std::make_unique<render_resource_cache>(renderer);
}

void render_view::set_local_transform(const vector3& location, const quat& rotation, const vector3& scale)
{
    render_object::set_local_transform(location, rotation, scale);

    update_view_info_param_block();
}

void render_view::set_viewport(const recti& viewport)
{
    m_viewport = viewport;

    update_view_info_param_block();
}

recti render_view::get_viewport()
{
    return m_viewport;
}

void render_view::set_clip(float near, float far)
{
    m_near_clip = near;
    m_far_clip = far;

    update_view_info_param_block();
}

void render_view::get_clip(float& near, float& far)
{
    near = m_near_clip;
    far = m_far_clip;
}

void render_view::set_fov(float fov)
{
    m_field_of_view = fov;

    update_view_info_param_block();
}

float render_view::get_fov()
{
    return m_field_of_view;
}

void render_view::set_aspect_ratio(float ratio)
{
    m_aspect_ratio = ratio;

    update_view_info_param_block();
}

float render_view::get_aspect_ratio()
{
    return m_aspect_ratio;
}

matrix4 render_view::get_view_matrix()
{
    return matrix4::look_at(
        m_local_location,
        m_local_location + (vector3::forward * m_local_rotation),
        vector3::up);
}

matrix4 render_view::get_perspective_matrix()
{
    return matrix4::perspective(
        math::radians(m_field_of_view),
        m_aspect_ratio,
        m_near_clip,
        m_far_clip);
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
    m_view_info_param_block->set("projection_matrix", get_perspective_matrix());
}

frustum render_view::get_frustum()
{
    return frustum(get_view_matrix() * get_perspective_matrix());
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

}; // namespace ws
