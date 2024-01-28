// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/windows/editor_viewport_window.h"
#include "workshop.editor/editor/editor.h"
#include "workshop.core/containers/string.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.assets/asset.h"
#include "workshop.engine/ecs/object_manager.h"
#include "workshop.engine/ecs/meta_component.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.engine/engine/world.h"

#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/transform/bounds_component.h"
#include "workshop.game_framework/components/camera/camera_component.h"
#include "workshop.game_framework/components/camera/editor_camera_movement_component.h"

#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.game_framework/systems/camera/camera_system.h"
#include "workshop.game_framework/systems/camera/editor_camera_movement_system.h"

#include "workshop.renderer/renderer.h"
#include "workshop.render_interface/ri_interface.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/imgui_internal.h"

namespace ws {

editor_viewport_window::editor_viewport_window(editor* in_editor, engine* in_engine, size_t index)
    : m_engine(in_engine)
    , m_editor(in_editor)
    , m_viewport_index(index)
{
    snprintf(m_name, sizeof(m_name), "Viewport %zi", m_viewport_index + 1);
    
    m_on_default_world_changed_delegate = m_engine->on_default_world_changed.add_shared([this](world* new_world) {
        recreate_views();
    });
}

editor_viewport_window::~editor_viewport_window()
{
    m_on_default_world_changed_delegate = nullptr;
}

void editor_viewport_window::recreate_views()
{
    auto& obj_manager = m_engine->get_default_world().get_object_manager();
    transform_system* transform_sys = obj_manager.get_system<transform_system>();
    camera_system* camera_sys = obj_manager.get_system<camera_system>();

    m_view_camera = obj_manager.create_object(string_format("%s Camera", m_name).c_str());
    obj_manager.add_component<transform_component>(m_view_camera);
    obj_manager.add_component<bounds_component>(m_view_camera);
    obj_manager.add_component<camera_component>(m_view_camera);
    obj_manager.add_component<editor_camera_movement_component>(m_view_camera);
    transform_sys->set_local_transform(m_view_camera, vector3::zero, quat::identity, vector3::one);

    meta_component* meta = obj_manager.get_component<meta_component>(m_view_camera);
    meta->flags = meta->flags | object_flags::transient | object_flags::hidden;

    camera_sys->set_draw_flags(m_view_camera, render_draw_flags::geometry | render_draw_flags::editor);

    update_render_target(true);
}

void editor_viewport_window::update_render_target(bool initial_update)
{
    renderer& render = m_engine->get_renderer();
    auto& obj_manager = m_engine->get_default_world().get_object_manager();
    camera_system* camera_sys = obj_manager.get_system<camera_system>();

    ImVec2 viewport_size = initial_update ? ImVec2(1, 1) : ImGui::GetContentRegionAvail();
    ImVec2 current_viewport_size;

    // Clean up old render targets.
    // TODO: This is pure jank, remove this.
    while (m_render_target_remove_queue.size() > 4)
    {
        m_render_target_remove_queue.erase(m_render_target_remove_queue.begin());
    }
    m_current_render_target = m_render_target.get();

    // Recreate render target if one doesn't exist yet or the size has changed.
    if (m_render_target == nullptr ||
        viewport_size.x != m_render_target->get_width() ||
        viewport_size.y != m_render_target->get_height())
    {
        // Keep old RT around as it may be in use on the render thread.
        if (m_render_target)
        {
            m_current_render_target = m_render_target.get();
            m_render_target_remove_queue.push_back(std::move(m_render_target));
        }

        // Create RT to render to.
        ri_texture::create_params create_params;
        create_params.dimensions = ri_texture_dimension::texture_2d;
        create_params.width = (size_t)viewport_size.x;
        create_params.height = (size_t)viewport_size.y;
        create_params.mip_levels = 1;
        create_params.is_render_target = true;
        create_params.format = ri_texture_format::R8G8B8A8_SRGB;
        m_render_target = render.get_render_interface().create_texture(create_params, m_name);

        // Update the render target for the camera.
        camera_sys->set_viewport(m_view_camera, recti(0, 0, (int)viewport_size.x, (int)viewport_size.y));
        camera_sys->set_projection(m_view_camera, 45.0f, viewport_size.x / viewport_size.y, 10.0f, 20000.0f);
        camera_sys->set_render_target(m_view_camera, m_render_target.get());
    }
}

void editor_viewport_window::draw()
{ 
    ImRect viewport_rect = ImRect(0, 0, 0, 0);
    bool mouse_over_viewport = false;
    bool input_blocked = false;

    auto& obj_manager = m_engine->get_default_world().get_object_manager();
    editor_camera_movement_system* camera_sys = obj_manager.get_system<editor_camera_movement_system>();
    editor_camera_movement_component* movement_comp = obj_manager.get_component<editor_camera_movement_component>(m_view_camera);
    camera_component* camera_comp = obj_manager.get_component<camera_component>(m_view_camera);

    if (m_open)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        if (ImGui::Begin(get_window_id(), &m_open))
        {
            update_render_target(false);

            if (m_current_render_target)
            {
                ImGui::Image(m_current_render_target, ImGui::GetContentRegionAvail());
            }
            
            ImVec2 window_pos = ImGui::GetWindowPos();

            ImVec2 window_min = ImGui::GetWindowContentRegionMin();
            ImVec2 window_max = ImGui::GetWindowContentRegionMax();
            
            viewport_rect = ImRect(
                ImVec2(window_pos.x + window_min.x, window_pos.y + window_min.y),
                ImVec2(window_pos.x + window_max.x, window_pos.y + window_max.y)
            );
            
            m_editor->draw_selection(camera_comp, viewport_rect, movement_comp->is_focused);
        }
        ImGui::End();

        ImGui::PopStyleVar();
    }

    // We contract the viewport a little bit to account for using splitters/etc which may move the cursor slightly
    // into the viewport but we don't wish to treat it as though it is.
    viewport_rect.Expand(-10.0f);
    mouse_over_viewport = ImGui::IsMouseHoveringRect(viewport_rect.Min, viewport_rect.Max, false);
    input_blocked = ImGuizmo::IsUsingAny() || ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopup);

    camera_sys->set_input_state(m_view_camera, recti(
        (int)(viewport_rect.Min.x),
        (int)(viewport_rect.Min.y),
        (int)(viewport_rect.Max.x - viewport_rect.Min.x),
        (int)(viewport_rect.Max.y - viewport_rect.Min.y)
    ), mouse_over_viewport, input_blocked);
}

const char* editor_viewport_window::get_window_id()
{
    return m_name;
}

editor_window_layout editor_viewport_window::get_layout()
{
    switch (m_viewport_index)
    {
        case 0:     return editor_window_layout::center_top_left;
        case 1:     return editor_window_layout::center_top_right;
        case 2:     return editor_window_layout::center_bottom_left;
        case 3:     return editor_window_layout::center_bottom_right;
        default:    return editor_window_layout::center_bottom_right;
    }    
}

}; // namespace ws
