// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/editor.h"
#include "workshop.editor/editor/editor_main_menu.h"
#include "workshop.editor/editor/windows/editor_log_window.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_imgui_manager.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/imgui_internal.h"

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
    create_window<editor_log_window>();

    return true;
}

result<void> editor::destroy_windows()
{
    m_windows.clear();

    return true;
}

void editor::step(const frame_time& time)
{
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

    auto dock_id_top = ImGui::DockBuilderSplitNode(m_dockspace_id, ImGuiDir_Up, 0.2f, nullptr, &m_dockspace_id);
    auto dock_id_bottom = ImGui::DockBuilderSplitNode(m_dockspace_id, ImGuiDir_Down, 0.25f, nullptr, &m_dockspace_id);
    auto dock_id_left = ImGui::DockBuilderSplitNode(m_dockspace_id, ImGuiDir_Left, 0.2f, nullptr, &m_dockspace_id);
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

}; // namespace ws
