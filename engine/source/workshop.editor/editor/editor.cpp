// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/editor.h"
#include "workshop.editor/editor/editor_main_menu.h"
#include "workshop.editor/editor/windows/editor_loading_window.h"
#include "workshop.editor/editor/windows/editor_log_window.h"
#include "workshop.editor/editor/windows/editor_memory_window.h"
#include "workshop.editor/editor/windows/editor_scene_tree_window.h"
#include "workshop.editor/editor/windows/editor_properties_window.h"
#include "workshop.editor/editor/windows/editor_assets_window.h"
#include "workshop.engine/engine/world.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_imgui_manager.h"
#include "workshop.engine/ecs/object_manager.h"
#include "workshop.engine/ecs/component.h"
#include "workshop.engine/ecs/component_filter.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/transform/bounds_component.h"
#include "workshop.game_framework/components/camera/camera_component.h"
#include "workshop.game_framework/systems/transform/bounds_system.h"
#include "workshop.game_framework/systems/transform/transform_system.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/imgui_internal.h"
#include "thirdparty/ImGuizmo/ImGuizmo.h"

#pragma optimize("", off)

namespace ws {

editor::editor(engine& in_engine)
	: m_engine(in_engine)
{
}

editor::~editor() = default;

void editor::register_init(init_list& list)
{
    list.add_step(
        "Editor Menu",
        [this, &list]() -> result<void> { return create_main_menu(list); },
        [this, &list]() -> result<void> { return destroy_main_menu(); }
    );
    list.add_step(
        "Editor Windows",
        [this, &list]() -> result<void> { return create_windows(list); },
        [this, &list]() -> result<void> { return destroy_windows(); }
    );
}

void editor::set_editor_mode(editor_mode mode)
{
	m_editor_mode = mode;
}

editor_main_menu& editor::get_main_menu()
{
    return *m_main_menu.get();
}

std::vector<object> editor::get_selected_objects()
{
    // TODO: Undo/Redo Stack
    return m_selected_objects;
}

void editor::set_selected_objects(std::vector<object>& objects)
{
    m_selected_objects = objects;
}

result<void> editor::create_main_menu(init_list& list)
{
    m_main_menu = std::make_unique<editor_main_menu>();
    
    // File Settings
    m_main_menu_options.push_back(m_main_menu->add_menu_item("File/New Scene", [this]() { 
        // TODO
    }));
    m_main_menu_options.push_back(m_main_menu->add_menu_item("File/Open Scene...", [this]() { 
        // TODO
    }));
    m_main_menu_options.push_back(m_main_menu->add_menu_seperator("File"));
    m_main_menu_options.push_back(m_main_menu->add_menu_item("File/Save Scene", [this]() { 
        // TODO
    }));
    m_main_menu_options.push_back(m_main_menu->add_menu_item("File/Save Scene As...", [this]() { 
        // TODO
    }));
    m_main_menu_options.push_back(m_main_menu->add_menu_seperator("File"));
    m_main_menu_options.push_back(m_main_menu->add_menu_item("File/Exit", [this]() { 
        // TODO
    }));

    // Build Settings
    m_main_menu_options.push_back(m_main_menu->add_menu_item("Build/Regenerate Diffuse Probes", [this]() { 
        m_engine.get_renderer().get_command_queue().regenerate_diffuse_probes();
    }));
    m_main_menu_options.push_back(m_main_menu->add_menu_item("Build/Regenerate Reflection Probes", [this]() {
        m_engine.get_renderer().get_command_queue().regenerate_reflection_probes();
    }));

    // Adding rendering options.
    for (size_t i = 0; i < static_cast<size_t>(visualization_mode::COUNT); i++)
    {
        std::string path = string_format("Render/Visualization/%s", visualization_mode_strings[i]);

        auto option = m_main_menu->add_menu_item(path.c_str(), [this, i]() {
            m_engine.get_renderer().get_command_queue().set_visualization_mode(static_cast<visualization_mode>(i));
        });

        m_main_menu_options.push_back(std::move(option));
    }

    m_main_menu_options.push_back(m_main_menu->add_menu_seperator("Render"));

    m_main_menu_options.push_back(m_main_menu->add_menu_item("Render/Toggle Cell Bounds", [this]() { 
        m_engine.get_renderer().get_command_queue().toggle_render_flag(render_flag::draw_cell_bounds);
    }));
    m_main_menu_options.push_back(m_main_menu->add_menu_item("Render/Toggle Object Bounds", [this]() { 
        m_engine.get_renderer().get_command_queue().toggle_render_flag(render_flag::draw_object_bounds);
    }));
    m_main_menu_options.push_back(m_main_menu->add_menu_item("Render/Toggle Direct Lighting", [this]() { 
        m_engine.get_renderer().get_command_queue().toggle_render_flag(render_flag::disable_direct_lighting);
    }));
    m_main_menu_options.push_back(m_main_menu->add_menu_item("Render/Toggle Ambient Lighting", [this]() { 
        m_engine.get_renderer().get_command_queue().toggle_render_flag(render_flag::disable_ambient_lighting);
    }));
    m_main_menu_options.push_back(m_main_menu->add_menu_item("Render/Toggle Freeze Rendering", [this]() { 
        m_engine.get_renderer().get_command_queue().toggle_render_flag(render_flag::freeze_rendering);
    }));

    // Window Settings
    m_main_menu_options.push_back(m_main_menu->add_menu_item("Window/Reset Layout", [this]() { 
        m_set_default_dock_space = false;
    }));    
    m_main_menu_options.push_back(m_main_menu->add_menu_seperator("Window"));
    m_main_menu_options.push_back(m_main_menu->add_menu_item("Window/Performance Overlay", [this]() { 
        m_engine.get_renderer().get_command_queue().toggle_render_flag(render_flag::draw_performance_overlay);
    }));
    m_main_menu_options.push_back(m_main_menu->add_menu_custom("Window/", [this]() {       
        for (auto& window : m_windows)
        {
            if (ImGui::MenuItem(window->get_window_id()))
            {
                window->open();
            }
        }
    }));

    return true;
}

result<void> editor::destroy_main_menu()
{
    m_main_menu = nullptr;

    return true;
}

result<void> editor::create_windows(init_list& list)
{
    create_window<editor_properties_window>(this, &m_engine.get_default_world());
    create_window<editor_scene_tree_window>(this, &m_engine.get_default_world());
    create_window<editor_loading_window>(&m_engine.get_asset_manager());
    create_window<editor_assets_window>(&m_engine.get_asset_manager(), &m_engine.get_asset_database());
    create_window<editor_log_window>();
    create_window<editor_memory_window>();

    return true;
}

result<void> editor::destroy_windows()
{
    m_windows.clear();

    return true;
}

void editor::step(const frame_time& time)
{
    profile_marker(profile_colors::engine, "editor");

	imgui_scope imgui_scope(m_engine.get_renderer().get_imgui_manager(), "Editor Dock");

	input_interface& input = m_engine.get_input_interface();

	// Switch between modes
	if (input.was_key_pressed(input_key::escape))
	{
		set_editor_mode(m_editor_mode == editor_mode::editor ? editor_mode::game : editor_mode::editor);
	}

	// Change input state depending on mode.
	bool needs_input = (m_editor_mode != editor_mode::editor);
	input.set_mouse_hidden(needs_input);
	input.set_mouse_capture(needs_input);

	// Draw relevant parts of the editor ui.
    ImGuiIO& imgui_io = ImGui::GetIO();
    ImRect viewport_rect = ImRect(ImVec2(0, 0), ImVec2(imgui_io.DisplaySize.x, imgui_io.DisplaySize.y));

	if (m_editor_mode == editor_mode::editor)
	{
		draw_dockspace();

        viewport_rect = ImGui::DockBuilderGetCentralNode(m_dockspace_id)->Rect();

        // Draw selection.
        ImGui::SetNextWindowPos(viewport_rect.Min);
        ImGui::SetNextWindowSize(ImVec2(viewport_rect.Max.x - viewport_rect.Min.x, viewport_rect.Max.y - viewport_rect.Min.y));
        ImGui::Begin("SelectionView", nullptr, ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoDecoration);
        draw_selection();
        ImGui::End();
	}

    // We contract the viewport a little bit to account for using splitters/etc which may move the cursor slightly
    // into the viewport but we don't wish to treat it as though it is.
    viewport_rect.Expand(-10.0f);

    m_engine.set_mouse_over_viewport(
        ImGui::IsMouseHoveringRect(viewport_rect.Min, viewport_rect.Max, false) && 
        !ImGuizmo::IsUsingAny() && 
        !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopup));
}

void editor::draw_dockspace()
{
	ImGuiWindowFlags window_flags = 
		ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | 
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | 
		ImGuiWindowFlags_NoNavFocus;
	
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::SetNextWindowBgAlpha(0.0f);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin("Dockspace", nullptr, window_flags);
		ImGui::PopStyleVar(3);

		if (ImGui::BeginMainMenuBar())
		{
            m_main_menu->draw();
		}
		ImGui::EndMainMenuBar();

		m_dockspace_id = ImGui::GetID("MainDockspace");
		ImGui::DockSpace(m_dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        if (!m_set_default_dock_space)
        {
            m_set_default_dock_space = true;
            reset_dockspace_layout();
        }
	ImGui::End();

    for (auto& window : m_windows)
    {
        window->draw();
    }    
}

void editor::reset_dockspace_layout()
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::DockBuilderRemoveNode(m_dockspace_id); 
    ImGui::DockBuilderAddNode(m_dockspace_id, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(m_dockspace_id, viewport->Size);

    auto dock_id_top = ImGui::DockBuilderSplitNode(m_dockspace_id, ImGuiDir_Up, 0.3f, nullptr, &m_dockspace_id);
    auto dock_id_bottom_left = ImGui::DockBuilderSplitNode(m_dockspace_id, ImGuiDir_Down, 0.3f, nullptr, &m_dockspace_id);
    auto dock_id_bottom_right = ImGui::DockBuilderSplitNode(dock_id_bottom_left, ImGuiDir_Right, 0.5f, nullptr, &dock_id_bottom_left);
    auto dock_id_left = ImGui::DockBuilderSplitNode(m_dockspace_id, ImGuiDir_Left, 0.15f, nullptr, &m_dockspace_id);
    auto dock_id_right = ImGui::DockBuilderSplitNode(m_dockspace_id, ImGuiDir_Right, 0.15f, nullptr, &m_dockspace_id);

    // we now dock our windows into the docking node we made above
    for (auto& window : m_windows)
    {
        ImGuiID dock_id = dock_id_left;
        switch (window->get_layout())
        {
        case editor_window_layout::top:
            {
                dock_id = dock_id_top;
                break;
            }
        case editor_window_layout::bottom_left:
            {
                dock_id = dock_id_bottom_left;
                break;
            }
        case editor_window_layout::bottom_right:
            {
                dock_id = dock_id_bottom_right;
                break;
            }
        case editor_window_layout::left:
            {
                dock_id = dock_id_left;
                break;
            }
        case editor_window_layout::right:
            {
                dock_id = dock_id_right;
                break;
            }
        }        

        ImGui::DockBuilderDockWindow(window->get_window_id(), dock_id);
    }

    ImGui::DockBuilderFinish(m_dockspace_id);
}

camera_component* editor::get_camera()
{
    component_filter<camera_component, const transform_component> filter(m_engine.get_default_world().get_object_manager());
    if (filter.size())
    {
        return filter.get_component<camera_component>(0);
    }
    return nullptr;
}

void editor::draw_selection()
{
    if (m_selected_objects.empty() || 
        m_editor_mode != editor_mode::editor)
    {
        return;
    }

    camera_component* camera = get_camera();

    renderer& render = m_engine.get_renderer();
    object_manager& obj_manager = m_engine.get_default_world().get_object_manager();
    bounds_system* bounds_sys = obj_manager.get_system<bounds_system>();
    transform_system* transform_sys = obj_manager.get_system<transform_system>();

    if (ImGui::IsKeyPressed(ImGuiKey_Space))
    {
        if (m_current_gizmo_mode == ImGuizmo::OPERATION::TRANSLATE)
        {
            m_current_gizmo_mode = ImGuizmo::OPERATION::ROTATE;
        }
        else if (m_current_gizmo_mode == ImGuizmo::OPERATION::ROTATE)
        {
            m_current_gizmo_mode = ImGuizmo::OPERATION::SCALE;
        }
        else if (m_current_gizmo_mode == ImGuizmo::OPERATION::SCALE)
        {
            m_current_gizmo_mode = ImGuizmo::OPERATION::TRANSLATE;
        }
    }

    ImGuizmo::SetDrawlist(nullptr);
    ImGuizmo::SetRect(0, 0, (float)render.get_display_width(), (float)render.get_display_height());

    bool fixed_pivot_point = (ImGuizmo::IsUsingAny() && m_current_gizmo_mode != ImGuizmo::OPERATION::TRANSLATE);
    obb selected_object_bounds = bounds_sys->get_combined_bounds(m_selected_objects, m_pivot_point, fixed_pivot_point);
    render.get_command_queue().draw_obb(selected_object_bounds, color::cyan);

    matrix4 view_mat = camera->view_matrix;
    matrix4 proj_mat = camera->projection_matrix;
    matrix4 model_mat = selected_object_bounds.transform;

    float view_mat_raw[16];
    float proj_mat_raw[16];
    float model_mat_raw[16];

    view_mat.get_raw(view_mat_raw, false);
    proj_mat.get_raw(proj_mat_raw, false);
    model_mat.get_raw(model_mat_raw, false);

    matrix4 world_to_pivot = selected_object_bounds.transform.inverse();

    if (!m_selected_object_states.empty() && ImGuizmo::Manipulate(view_mat_raw, proj_mat_raw, m_current_gizmo_mode, ImGuizmo::MODE::LOCAL, model_mat_raw))
    {
        matrix4 model_mat;
        model_mat.set_raw(model_mat_raw, false);

        vector3 world_location;
        vector3 world_scale;
        vector3 world_rotation_euler;
        quat world_rotation;
        model_mat.decompose(&world_location, &world_rotation_euler, &world_scale);
        world_rotation = quat::euler(world_rotation_euler);

        matrix4 world_to_new_pivot = model_mat.inverse();
        matrix4 new_pivot_to_world = model_mat;

        for (size_t i = 0; i < m_selected_objects.size(); i++)
        {
            object obj = m_selected_objects[i];
            object_state& state = m_selected_object_states[i];
            transform_component* comp = obj_manager.get_component<transform_component>(obj);

            // move object transform from world space to original bounds space
            matrix4 object_to_world = 
                //matrix4::scale(comp->world_scale) *           // We don't need to handle scale for this, and if we do we end up in a feedback loop.
                matrix4::rotation(comp->world_rotation) *
                matrix4::translate(comp->world_location);

            matrix4 relative = object_to_world * world_to_pivot;
             
            // move object transform from original bounds space to world space using the new transform.
            matrix4 new_object_world = relative * new_pivot_to_world;;

            vector3 new_location;
            vector3 new_scale_global;
            vector3 new_rotation_euler;

            new_object_world.decompose(&new_location, &new_rotation_euler, &new_scale_global);

            quat new_rotation = quat::euler(new_rotation_euler);
            vector3 new_scale = state.original_scale * new_scale_global;

            // Apply changes.
            // We shouldn't need to do this as seperate operations, but imguizmo unfortunately zeros out the rotation
            // when using the scale operation.
            if (m_current_gizmo_mode == ImGuizmo::OPERATION::TRANSLATE ||
                m_current_gizmo_mode == ImGuizmo::OPERATION::ROTATE)
            {
                transform_sys->set_world_transform(obj, new_location, new_rotation, comp->world_scale);
            }
            else if (m_current_gizmo_mode == ImGuizmo::OPERATION::SCALE)
            {
                transform_sys->set_world_transform(obj, comp->world_location, comp->world_rotation, new_scale);
            }
        }
    }

    if (!ImGuizmo::IsUsingAny())
    {
        m_selected_object_states.clear();
        for (size_t i = 0; i < m_selected_objects.size(); i++)
        {
            object obj = m_selected_objects[i];
            object_state& state = m_selected_object_states.emplace_back();
            transform_component* comp = obj_manager.get_component<transform_component>(obj);

            state.original_scale = comp->world_scale;
        }
    }
}

}; // namespace ws
