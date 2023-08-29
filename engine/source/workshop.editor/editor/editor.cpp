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
	if (m_editor_mode == editor_mode::editor)
	{
		draw_dockspace();
	}

    // Draw selection.
    draw_selection();
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
    auto dock_id_bottom = ImGui::DockBuilderSplitNode(m_dockspace_id, ImGuiDir_Down, 0.3f, nullptr, &m_dockspace_id);
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
        case editor_window_layout::bottom:
            {
                dock_id = dock_id_bottom;
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

#pragma optimize("", off)

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

    ImGuizmo::SetRect(0, 0, (float)render.get_display_width(), (float)render.get_display_height());

    render.get_command_queue().draw_obb(m_selected_objects_obb, color::cyan);

    matrix4 view_mat = camera->view_matrix;
    matrix4 proj_mat = camera->projection_matrix;
    matrix4 model_mat = m_selected_objects_obb.transform;

    float view_mat_raw[16];
    float proj_mat_raw[16];
    float model_mat_raw[16];

    view_mat.get_raw(view_mat_raw, false);
    proj_mat.get_raw(proj_mat_raw, false);
    model_mat.get_raw(model_mat_raw, false);

    if (!m_selected_object_state.empty() && ImGuizmo::Manipulate(view_mat_raw, proj_mat_raw, m_current_gizmo_mode, ImGuizmo::MODE::WORLD, model_mat_raw))
    {
        float translation_raw[3];
        float rotation_raw[3];
        float scale_raw[3];
        ImGuizmo::DecomposeMatrixToComponents(model_mat_raw, translation_raw, rotation_raw, scale_raw);

        vector3 world_location = vector3(translation_raw[0], translation_raw[1], translation_raw[2]);
        vector3 world_scale = vector3(scale_raw[0], scale_raw[1], scale_raw[2]);
        quat world_rotation = quat::euler(vector3(math::radians(rotation_raw[0]), math::radians(rotation_raw[1]), math::radians(rotation_raw[2])));

        for (size_t i = 0; i < m_selected_objects.size(); i++)
        {
            object obj = m_selected_objects[i];
            object_state& obj_state = m_selected_object_state[i];
            transform_component* comp = obj_manager.get_component<transform_component>(obj);

            // This is super annoying, we shouldn't need to do this but ImGuizmo zeros out the rotation when
            // using scale mode. So to work around this we only modify the elements for the relevant mode.
            if (m_current_gizmo_mode == ImGuizmo::OPERATION::TRANSLATE)
            {
                vector3 bounds_origin = m_selected_objects_obb.transform.transform_location(vector3::zero);
                vector3 relative_offset = (comp->world_location - bounds_origin);
                transform_sys->set_world_transform(obj, world_location + relative_offset, comp->world_rotation, comp->world_scale);
            }
            else if (m_current_gizmo_mode == ImGuizmo::OPERATION::ROTATE)
            {
                //quat delta = quat::rotate_to(vector3::forward * world_rotation, bounds.transform.transform_direction(vector3::forward));
                transform_sys->set_world_transform(obj, comp->world_location, world_rotation/* * comp->world_rotation*/, comp->world_scale);
            }
            else if (m_current_gizmo_mode == ImGuizmo::OPERATION::SCALE)
            {
                db_log(core, "local_scale=%.2f,%.2f,%.2f world_scale=%.2f,%.2f,%.2f", 
                    obj_state.local_scale.x,
                    obj_state.local_scale.y,
                    obj_state.local_scale.z,
                    world_scale.x,
                    world_scale.y,
                    world_scale.z);
                vector3 delta = (world_scale - m_selected_objects_obb.transform.extract_scale());
                transform_sys->set_world_transform(obj, comp->world_location, comp->world_rotation, comp->world_scale + delta);
            }
        }

        m_selected_objects_obb.transform.set_raw(model_mat_raw, false);
    }
    else
    {
        m_selected_object_state.clear();
        for (object obj : m_selected_objects)
        {
            transform_component* comp = obj_manager.get_component<transform_component>(obj);
            
            object_state& state = m_selected_object_state.emplace_back();
            state.local_location = comp->local_location;
            state.local_rotation = comp->local_rotation;
            state.local_scale = comp->local_scale;
        }

        m_selected_objects_obb = bounds_sys->get_combined_bounds(m_selected_objects);
    }
}

}; // namespace ws
