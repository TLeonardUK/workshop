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
#include "workshop.editor/editor/windows/popups/editor_progress_popup.h"
#include "workshop.editor/editor/transactions/editor_transaction_change_selected_objects.h"
#include "workshop.editor/editor/transactions/editor_transaction_change_object_transform.h"

#include "workshop.engine/engine/world.h"
#include "workshop.engine/assets/scene/scene.h"
#include "workshop.engine/assets/scene/scene_loader.h"
#include "workshop.engine/ecs/object_manager.h"
#include "workshop.engine/ecs/component.h"
#include "workshop.engine/ecs/component_filter.h"

#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_imgui_manager.h"

#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/transform/bounds_component.h"
#include "workshop.game_framework/components/camera/camera_component.h"
#include "workshop.game_framework/components/camera/fly_camera_movement_component.h"
#include "workshop.game_framework/systems/transform/bounds_system.h"
#include "workshop.game_framework/components/lighting/directional_light_component.h"
#include "workshop.game_framework/components/geometry/static_mesh_component.h"
#include "workshop.game_framework/systems/default_systems.h"
#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.game_framework/systems/camera/fly_camera_movement_system.h"
#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.game_framework/systems/lighting/directional_light_system.h"
#include "workshop.game_framework/systems/geometry/static_mesh_system.h"

#include "workshop.core/platform/platform.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/imgui_internal.h"
#include "thirdparty/ImGuizmo/ImGuizmo.h"

namespace ws {

editor::editor(engine& in_engine)
	: m_engine(in_engine)
{
    m_undo_stack = std::make_unique<editor_undo_stack>();
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
    list.add_step(
        "Editor World",
        [this, &list]() -> result<void> { return create_world(list); },
        [this, &list]() -> result<void> { return destroy_world(); }
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
    return m_selected_objects;
}

void editor::set_selected_objects(const std::vector<object>& objects)
{
    m_undo_stack->push(std::make_unique<editor_transaction_change_selected_objects>(*this, objects));
}

void editor::set_selected_objects_untransacted(const std::vector<object>& objects)
{
    world& world_instance = m_engine.get_default_world();
    object_manager& obj_manager = world_instance.get_object_manager();
    static_mesh_system* static_mesh_sys = obj_manager.get_system<static_mesh_system>();

    // TODO: This isn't very extensible, we should be targetting some kind of base mesh_component instead
    // of doing static meshes/etc here.

    // Turn off selection flag for all old object meshes.
    for (object obj : m_selected_objects)
    {
        static_mesh_component* mesh = obj_manager.get_component<static_mesh_component>(obj);
        if (mesh)
        {
            static_mesh_sys->set_render_gpu_flags(obj, mesh->render_gpu_flags & ~render_gpu_flags::selected);
        }
    }

    m_selected_objects = objects;

    // Turn on selection flag for all new object meshes.
    for (object obj : m_selected_objects)
    {
        static_mesh_component* mesh = obj_manager.get_component<static_mesh_component>(obj);
        if (mesh)
        {
            static_mesh_sys->set_render_gpu_flags(obj, mesh->render_gpu_flags | render_gpu_flags::selected);
        }
    }
}

result<void> editor::create_main_menu(init_list& list)
{
    m_main_menu = std::make_unique<editor_main_menu>();
    
    // File Settings
    m_main_menu_options.push_back(m_main_menu->add_menu_item("File/New Scene", [this]() { 
        new_scene();
    }));
    m_main_menu_options.push_back(m_main_menu->add_menu_item("File/Open Scene...", [this]() { 
        open_scene();
    }));
    m_main_menu_options.push_back(m_main_menu->add_menu_seperator("File"));
    m_main_menu_options.push_back(m_main_menu->add_menu_item("File/Save Scene", [this]() { 
        save_scene(false);
    }));
    m_main_menu_options.push_back(m_main_menu->add_menu_item("File/Save Scene As...", [this]() { 
        save_scene(true);
    }));
    m_main_menu_options.push_back(m_main_menu->add_menu_seperator("File"));
    m_main_menu_options.push_back(m_main_menu->add_menu_item("File/Exit", [this]() { 
        app::instance().quit();
    }));

    // Edit Settings    
    m_main_menu_options.push_back(m_main_menu->add_menu_custom("Edit/", [this]() {       

        std::string undo_name = m_undo_stack->get_next_undo_name();
        if (ImGui::MenuItem(("Undo: " + undo_name).c_str(), nullptr, false, !undo_name.empty()))
        {
            m_undo_stack->undo();
        }

        std::string redo_name = m_undo_stack->get_next_redo_name();
        if (ImGui::MenuItem(("Redo: " + redo_name).c_str(), nullptr, false, !redo_name.empty()))
        {
            m_undo_stack->redo();
        }

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
    m_main_menu_options.push_back(m_main_menu->add_menu_item("Window/Performance", [this]() { 
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
    create_window<editor_properties_window>(this, &m_engine);
    create_window<editor_scene_tree_window>(this, &m_engine);
    create_window<editor_loading_window>(&m_engine.get_asset_manager());
    create_window<editor_assets_window>(&m_engine.get_asset_manager(), &m_engine.get_asset_database());
    create_window<editor_log_window>();
    create_window<editor_memory_window>();
    create_window<editor_progress_popup>();

    return true;
}

result<void> editor::destroy_windows()
{
    m_windows.clear();

    return true;
}

result<void> editor::create_world(init_list& list)
{
    new_scene();
    return true;
}

result<void> editor::destroy_world()
{
    // Nothing to do here, the original world created may have been destroyed
    // or swapped for another by this point. Destroying it is handled by the engine.
    return true;
}

editor_undo_stack& editor::get_undo_stack()
{
    return *m_undo_stack;
}

void editor::new_scene()
{
    world* new_world = m_engine.create_world("Default World");

    auto& obj_manager = new_world->get_object_manager();

    transform_system* transform_sys = obj_manager.get_system<transform_system>();
    directional_light_system* direction_light_sys = obj_manager.get_system<directional_light_system>();

    // Add a movement camera.
    object obj = obj_manager.create_object("main camera");
    obj_manager.add_component<transform_component>(obj);
    obj_manager.add_component<bounds_component>(obj);
    obj_manager.add_component<camera_component>(obj);
    obj_manager.add_component<fly_camera_movement_component>(obj);
    transform_sys->set_local_transform(obj, vector3(0.0f, 100.0f, -250.0f), quat::identity, vector3::one);

    // Add a directional light.
    obj = obj_manager.create_object("sun light");
    obj_manager.add_component<transform_component>(obj);
    obj_manager.add_component<bounds_component>(obj);
    obj_manager.add_component<directional_light_component>(obj);
    direction_light_sys->set_light_shadow_casting(obj, true);
    direction_light_sys->set_light_shadow_map_size(obj, 2048);
    direction_light_sys->set_light_shadow_max_distance(obj, 10000.0f);
    direction_light_sys->set_light_shadow_cascade_exponent(obj, 0.6f);
    direction_light_sys->set_light_intensity(obj, 5.0f);
    transform_sys->set_local_transform(obj, vector3(0.0f, 300.0f, 0.0f), quat::angle_axis(-math::halfpi * 0.85f, vector3::right) * quat::angle_axis(0.5f, vector3::forward), vector3::one);

    // Switch to the new default world.
    m_engine.set_default_world(new_world);

    // Clear out any state from the old world.
    reset_scene_state();
}

void editor::reset_scene_state()
{
    m_selected_objects.clear();
    m_selected_object_states.clear();
    m_undo_stack->clear();
}

void editor::open_scene()
{
    std::vector<file_dialog_filter> filter;
    filter.push_back({ "Scene Asset", { "yaml" }});

    if (std::string path = open_file_dialog("Open Scene", filter); !path.empty())
    {
        std::string vfs_path = virtual_file_system::get().get_vfs_location(path.c_str());
        if (!vfs_path.empty())
        {
            editor_progress_popup* popup = get_window<editor_progress_popup>();
            popup->set_title("Loading Scene");
            popup->set_subtitle(vfs_path.c_str());
            popup->set_progress(0.5f);
            popup->open();

            m_pending_open_scene = m_engine.get_asset_manager().request_asset<scene>(vfs_path.c_str(), 0);            
        }
        else
        {
            message_dialog("Failed to load scene asset. Asset is not stored in the virtual file system, please ensure its in the correct folder.", message_dialog_type::error);
        }
    }
}

void editor::commit_scene_load()
{
    if (m_pending_open_scene.is_loaded())
    {
        m_engine.set_default_world(m_pending_open_scene->world_instance);

        // Null out the assets world so it won't be unloaded when the scene asset falls out of scope.
        m_pending_open_scene->world_instance = nullptr;

        // Clear out any state from the old world.
        reset_scene_state();

        // Close the progression popup.
        get_window<editor_progress_popup>()->close();

        m_current_scene_path = m_pending_open_scene.get_path();
    }
    else
    {
        message_dialog("Failed to load scene asset. See log for more details.", message_dialog_type::error);
    }
}

void editor::save_scene(bool ask_for_filename)
{
    std::string path = m_current_scene_path;

    if (path.empty() || ask_for_filename)
    {
        std::vector<file_dialog_filter> filter;
        filter.push_back({ "Scene Asset", { "yaml" } });

        if (path = save_file_dialog("Open Scene", filter); path.empty())
        {
            return;
        }
    }

    std::string vfs_path = virtual_file_system::get().get_vfs_location(path.c_str());
    if (vfs_path.empty())
    {
        message_dialog("Failed to save scene asset. Asset is not stored in the virtual file system, please ensure its in the correct folder.", message_dialog_type::error);
        return;
    }

    // Disable stepping the world while we save.
    m_engine.get_default_world().set_step_enabled(false);

    // Show progress dialog
    editor_progress_popup* popup = get_window<editor_progress_popup>();
    popup->set_title("Saving Scene");
    popup->set_subtitle(vfs_path.c_str());
    popup->set_progress(0.5f);
    popup->open();

    // Queue up a save to run in the background.
    m_pending_save_scene_success = false;
    m_pending_save_scene = async("Save Scene", task_queue::standard, [this, vfs_path]() {

        scene saved_scene(m_engine.get_asset_manager(), &m_engine);
        saved_scene.world_instance = &m_engine.get_default_world();

        // Use the host protocol to just read/write direct to disk location.
        asset_loader* loader = m_engine.get_asset_manager().get_loader_for_type<scene>();
        if (loader->save_uncompiled(vfs_path.c_str(), saved_scene))
        {
            m_current_scene_path = vfs_path;
            m_pending_save_scene_success = true;
        }

        // Null out the assets world so it won't be unloaded when the scene asset falls out of scope.
        saved_scene.world_instance = nullptr;

    });
}

void editor::commit_scene_save()
{
    if (!m_pending_save_scene_success)
    {
        message_dialog("Failed to save scene asset. See log for more details.", message_dialog_type::error);
    }

    // Close the progression popup.
    get_window<editor_progress_popup>()->close();

    // Ree-nable stepping the world.
    m_engine.get_default_world().set_step_enabled(true);
}

void editor::process_pending_save_load()
{
    if (m_pending_open_scene.is_valid())
    {
        if (m_pending_open_scene.get_state() == asset_loading_state::loaded ||
            m_pending_open_scene.get_state() == asset_loading_state::failed)
        {
            commit_scene_load();

            m_pending_open_scene.reset();
        }
    }

    if (m_pending_save_scene.is_valid())
    {
        if (m_pending_save_scene.is_complete())
        {
            commit_scene_save();

            m_pending_save_scene.reset();
        }
    }
}

void editor::step(const frame_time& time)
{
    profile_marker(profile_colors::engine, "editor");

	imgui_scope imgui_scope(m_engine.get_renderer().get_imgui_manager(), "Editor Dock");

	input_interface& input = m_engine.get_input_interface();

    process_pending_save_load();

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
        case editor_window_layout::popup:
            {
                // We don't want to dock popup windows.
                continue;
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
    render.get_command_queue().draw_obb(selected_object_bounds, color::gold);

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
        // Create a transaction for the transformation so we can undo/redo it.
        if (m_was_transform_objects && m_selected_object_states.size() > 0)
        {
            std::unique_ptr<editor_transaction_change_object_transform> transaction = std::make_unique<editor_transaction_change_object_transform>(m_engine, *this);

            for (size_t i = 0; i < m_selected_objects.size(); i++)
            {
                object obj = m_selected_objects[i];
                object_state& state = m_selected_object_states[i];
                transform_component* comp = obj_manager.get_component<transform_component>(obj);

                transaction->add_object(
                    obj,
                    state.original_location, state.original_rotation, state.original_scale,
                    comp->world_location, comp->world_rotation, comp->world_scale
                );
            }

            m_undo_stack->push(std::move(transaction));
        }

        // Update the selected states to the current transform.
        m_selected_object_states.clear();
        for (size_t i = 0; i < m_selected_objects.size(); i++)
        {
            object obj = m_selected_objects[i];
            object_state& state = m_selected_object_states.emplace_back();
            transform_component* comp = obj_manager.get_component<transform_component>(obj);

            state.original_scale = comp->world_scale;
            state.original_location = comp->world_location;
            state.original_rotation = comp->world_rotation;
        }

        m_was_transform_objects = false;
    }
    else
    {
        m_was_transform_objects = true;
    }
}

}; // namespace ws
