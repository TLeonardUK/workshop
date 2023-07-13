// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/utils/init_list.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.input_interface/input_interface.h"
#include "workshop.platform_interface/platform_interface.h"
#include "workshop.window_interface/window_interface.h"
#include "workshop.engine/presentation/presenter.h"
#include "workshop.debug_menu/debug_menu.h"
#include "workshop.core/utils/frame_time.h"
#include "workshop.core/utils/singleton.h"
#include "workshop.core/utils/event.h"

#include <vector>
#include <memory>
#include <filesystem>

namespace ws {

class world;
class presentation;
class presenter;
class renderer;
class asset_manager;
class virtual_file_system;
class statistics_manager;
class task_scheduler;

// ================================================================================================
//  This is the core engine class, its responsible for owning all the individual pieces (such as
//  all worlds, the renderer, etc) that make a game run, and binding them into something coherent.
// ================================================================================================
class engine 
    : public singleton<engine>
{
public:

    engine();
    ~engine();

    // Registers all the steps required to initialize the engine.
    // Interacting with this class without successfully running these steps is undefined.
    void register_init(init_list& list);

    // Advances the state of the engine. This should be called repeatedly in the main loop of the 
    // application. 
    // 
    // Calling this does not guarantee a new render frame, or the state of the simulation being advanced, 
    // as the engine runs entirely asyncronously. This function mearly provides the opportunity for the engine
    // to advance if its in a state that permits it.
    void step();

    // Gets the renderer instance.
    ri_interface& get_render_interface();

    // Gets the input instance.
    input_interface& get_input_interface();

    // Gets the platform instance.
    platform_interface& get_platform_interface();

    // Gets the renderer.
    renderer& get_renderer();

    // Gets the asset manager.
    asset_manager& get_asset_manager();

    // Gets the statistics manager.
    statistics_manager& get_statistics_manager();

    // Gets the windowing manager.
    window_interface& get_windowing();

    // Gets the main rendering window.
    window& get_main_window();

    // Gets the debug menu.
    debug_menu& get_debug_menu();

    // Gets the main filesystem.
    virtual_file_system& get_filesystem();

    // Gets the root folder that contains the engines assets.
    std::filesystem::path get_engine_asset_dir();

    // Gets the root folder that contains the game assets.
    std::filesystem::path get_game_asset_dir();

    // Gets the root folder that contains cached assets.
    std::filesystem::path get_asset_cache_dir();

    // Sets the renderer system to use, immutable once engine is initialized.
    void set_render_interface_type(ri_interface_type type);

    // Sets the windowing system to use, immutable once engine is initialized.
    void set_window_interface_type(window_interface_type type);

    // Sets the input system to use, immutable once engine is initialized.
    void set_input_interface_type(input_interface_type type);

    // Sets the platform system to use, immutable once engine is initialized.
    void set_platform_interface_type(platform_interface_type type);

    // Sets the initial window mode, should be set during configuration.
    void set_window_mode(const std::string& title, size_t width, size_t height, window_mode mode);

    // Invoked when the engine is stepped.
    event<frame_time> on_step;

private:

    result<void> create_task_scheduler(init_list& list);
    result<void> destroy_task_scheduler();

    result<void> create_window_interface(init_list& list);
    result<void> destroy_window_interface();

    result<void> create_render_interface(init_list& list);
    result<void> destroy_render_interface();

    result<void> create_renderer(init_list& list);
    result<void> destroy_renderer();

    result<void> create_input_interface(init_list& list);
    result<void> destroy_input_interface();

    result<void> create_platform_interface(init_list& list);
    result<void> destroy_platform_interface();

    result<void> create_main_window(init_list& list);
    result<void> destroy_main_window();

    result<void> create_presenter(init_list& list);
    result<void> destroy_presenter();

    result<void> create_debug_menu(init_list& list);
    result<void> destroy_debug_menu();

    result<void> create_filesystem(init_list& list);
    result<void> destroy_filesystem();

    result<void> create_statistics_manager(init_list& list);
    result<void> destroy_statistics_manager();

    result<void> create_asset_manager(init_list& list);
    result<void> destroy_asset_manager();

protected:

    std::vector<std::unique_ptr<world>> m_worlds;

    std::unique_ptr<window_interface> m_window_interface;
    std::unique_ptr<ri_interface> m_render_interface;
    std::unique_ptr<input_interface> m_input_interface;
    std::unique_ptr<platform_interface> m_platform_interface;

    std::unique_ptr<task_scheduler> m_task_scheduler;
    std::unique_ptr<renderer> m_renderer;
    std::unique_ptr<window> m_window;
    std::unique_ptr<presenter> m_presenter;
    std::unique_ptr<virtual_file_system> m_filesystem;
    std::unique_ptr<statistics_manager> m_statistics;
    std::unique_ptr<asset_manager> m_asset_manager;
    std::unique_ptr<debug_menu> m_debug_menu;

    ri_interface_type m_render_interface_type = ri_interface_type::dx12;
    window_interface_type m_window_interface_type = window_interface_type::sdl;
    input_interface_type m_input_interface_type = input_interface_type::sdl;
    platform_interface_type m_platform_interface_type = platform_interface_type::sdl;

    frame_time m_frame_time = {};

    std::string m_window_title = "Workshop Game";
    size_t m_window_width = 1280;
    size_t m_window_height = 720;
    window_mode m_window_mode = window_mode::windowed;

    std::filesystem::path m_engine_asset_dir;
    std::filesystem::path m_game_asset_dir;
    std::filesystem::path m_asset_cache_dir;

};

}; // namespace ws
