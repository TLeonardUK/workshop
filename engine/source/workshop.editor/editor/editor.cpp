// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/editor.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_imgui_manager.h"

#include "thirdparty/imgui/imgui.h"

namespace ws {

editor::editor(engine& in_engine)
	: m_engine(in_engine)
{
}

editor::~editor() = default;

void editor::register_init(init_list& list)
{
}

void editor::set_editor_mode(editor_mode mode)
{
	m_editor_mode = mode;
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
			draw_main_menu();
		}
		ImGui::EndMainMenuBar();

		ImGuiID dockspace_id = ImGui::GetID("MainDockspace");
		ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	ImGui::End();

	ImGui::Begin("Test Window");
	ImGui::Text("test....");
	ImGui::End();
}

void editor::draw_main_menu()
{
	debug_menu& debug_menu_instance = m_engine.get_debug_menu();

	// Merge any debug menu changes into the main menu.

	//debug_menu_instance.register();

	/*
	if (ImGui::BeginMenu("Test"))
	{
		ImGui::EndMenu();
	}
	*/
}

}; // namespace ws
