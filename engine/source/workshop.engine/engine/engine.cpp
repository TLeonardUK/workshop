// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.engine/assets/asset_database.h"
#include "workshop.engine/assets/scene/scene_loader.h"

#include "workshop.core/utils/init_list.h"
#include "workshop.core/perf/profile.h"
#include "workshop.core/async/task_scheduler.h"
#include "workshop.core/async/async.h"
#include "workshop.core/statistics/statistics_manager.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/filesystem/virtual_file_system_disk_handler.h"
#include "workshop.core/filesystem/virtual_file_system_redirect_handler.h"
#include "workshop.core/filesystem/file.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/filesystem/async_io_manager.h"
#include "workshop.core/app/app.h"
#include "workshop.core/statistics/statistics_manager.h"
#include "workshop.core/perf/timer.h"

#include "workshop.core/cvar/cvar.h"
#include "workshop.core/cvar/cvar_manager.h"

#include "workshop.assets/asset_manager.h"
#include "workshop.assets/caches/asset_cache_disk.h"

#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_effect.h"
#include "workshop.renderer/assets/shader/shader.h"
#include "workshop.renderer/assets/shader/shader_loader.h"
#include "workshop.renderer/assets/material/material.h"

#include "workshop.window_interface/window_interface.h"
#include "workshop.window_interface.sdl/sdl_window_interface.h"

#include "workshop.input_interface/input_interface.h"
#include "workshop.input_interface.sdl/sdl_input_interface.h"

#include "workshop.platform_interface/platform_interface.h"
#include "workshop.platform_interface.sdl/sdl_platform_interface.h"

#include "workshop.physics_interface/physics_interface.h"
#include "workshop.physics_interface.jolt/jolt_physics_interface.h"

#include "workshop.render_interface/ri_interface.h"

#ifdef WS_WINDOWS
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#endif

namespace ws {

engine::engine() = default;
engine::~engine() = default;

void engine::step()
{
    profile_marker(profile_colors::engine, "frame %zi", (size_t)m_frame_time.frame_count);
    profile_variable(m_frame_time.delta_seconds, "delta seconds");

    timer frame_timer;
    frame_timer.start();

    m_frame_time.step();

    {
        profile_marker(profile_colors::engine, "pump platform events");

        m_platform_interface->pump_events();
        m_window_interface->pump_events();
        m_input_interface->pump_events();
    }

    {
        profile_marker(profile_colors::engine, "game step");
        on_step.broadcast(m_frame_time);
    }

    for (auto& world : m_worlds)
    {
        world->step(m_frame_time);
    }

    m_editor->step(m_frame_time);
    m_presenter->step(m_frame_time);

    m_filesystem->raise_watch_events();

    // If any hot reloads are pending then drain the renderer and swap them.
    if (m_asset_manager->has_pending_hot_reloads())
    {
        m_renderer->pause();
        m_asset_manager->apply_hot_reloads();
        m_renderer->resume();
    }

    // Clean up any old cached asset database info.
    m_asset_database->clean_cache();

    frame_timer.stop();
    m_stats_frame_time_game->submit(frame_timer.get_elapsed_seconds());
    m_stats_frame_rate->submit(1.0 / m_frame_time.delta_seconds);

    // Swap out the current world if its completed loading.
    if (m_loading_world.is_loaded())
    {
        db_log(engine, "World loaded in %.2f ms: %s", (get_seconds() - m_loading_world_start_time) * 1000.0f, m_loading_world.get_path().c_str());

        set_default_world(m_loading_world->world_instance);

        // Null out of the world so its not destroyed when the asset_ptr is reset.
        m_loading_world->world_instance = nullptr;

        m_loading_world.reset();
    }

    // Commit engine statistics.
    statistics_manager::get().commit(statistics_commit_point::end_of_game);
}

void engine::register_init(init_list& list)
{
    list.add_step(
        "Task Scheduler",
        [this, &list]() -> result<void> { return create_task_scheduler(list); },
        [this, &list]() -> result<void> { return destroy_task_scheduler(); }
    );
    list.add_step(
        "Statistics Manager",
        [this, &list]() -> result<void> { return create_statistics_manager(list); },
        [this, &list]() -> result<void> { return destroy_statistics_manager(); }
    );
    list.add_step(
        "Filesystem",
        [this, &list]() -> result<void> { return create_filesystem(list); },
        [this, &list]() -> result<void> { return destroy_filesystem(); }
    );
    list.add_step(
        "Load Config",
        [this, &list]() -> result<void> { return load_config(list); },
        [this, &list]() -> result<void> { return true; }
    );
    list.add_step(
        "Asset Manager",
        [this, &list]() -> result<void> { return create_asset_manager(list); },
        [this, &list]() -> result<void> { return destroy_asset_manager(); }
    );
    list.add_step(
        "Register Engine Asset Loaders",
        [this, &list]() -> result<void> { return register_asset_loaders(list); },
        [this, &list]() -> result<void> { return unregister_asset_loaders(); }
    );
    list.add_step(
        "Platform Interface",
        [this, &list]() -> result<void> { return create_platform_interface(list); },
        [this, &list]() -> result<void> { return destroy_platform_interface(); }
    );
    list.add_step(
        "Window Interface",
        [this, &list]() -> result<void> { return create_window_interface(list); },
        [this, &list]() -> result<void> { return destroy_window_interface(); }
    );
    list.add_step(
        "Main Window",
        [this, &list]() -> result<void> { return create_main_window(list); },
        [this, &list]() -> result<void> { return destroy_main_window(); }
    );
    list.add_step(
        "Render Interface",
        [this, &list]() -> result<void> { return create_render_interface(list); },
        [this, &list]() -> result<void> { return destroy_render_interface(); }
    );
    list.add_step(
        "Input Interface",
        [this, &list]() -> result<void> { return create_input_interface(list); },
        [this, &list]() -> result<void> { return destroy_input_interface(); }
    );
    list.add_step(
        "Physics Interface",
        [this, &list]() -> result<void> { return create_physics_interface(list); },
        [this, &list]() -> result<void> { return destroy_physics_interface(); }
    );
    list.add_step(
        "Renderer",
        [this, &list]() -> result<void> { return create_renderer(list); },
        [this, &list]() -> result<void> { return destroy_renderer(); }
    );
    list.add_step(
        "Default World",
        [this, &list]() -> result<void> { return create_default_world(list); },
        [this, &list]() -> result<void> { return destroy_default_world(); }
    );
    list.add_step(
        "Presentation",
        [this, &list]() -> result<void> { return create_presenter(list); },
        [this, &list]() -> result<void> { return destroy_presenter(); }
    );
    list.add_step(
        "Editor",
        [this, &list]() -> result<void> { return create_editor(list); },
        [this, &list]() -> result<void> { return destroy_editor(); }
    );
}

ri_interface& engine::get_render_interface()
{
    return *m_render_interface.get();
}

input_interface& engine::get_input_interface()
{
    return *m_input_interface.get();
}

physics_interface& engine::get_physics_interface()
{
    return *m_physics_interface.get();
}

platform_interface& engine::get_platform_interface()
{
    return *m_platform_interface.get();
}

renderer& engine::get_renderer()
{
    return *m_renderer.get();
}

asset_manager& engine::get_asset_manager()
{
    return *m_asset_manager.get();
}

asset_database& engine::get_asset_database()
{
    return *m_asset_database.get();
}

statistics_manager& engine::get_statistics_manager()
{
    return *m_statistics.get();
}

window_interface& engine::get_windowing()
{
    return *m_window_interface.get();
}

window& engine::get_main_window()
{
    return *m_window.get();
}

editor& engine::get_editor()
{
    return *m_editor.get();
}

virtual_file_system& engine::get_filesystem()
{
    return *m_filesystem.get();
}

async_io_manager& engine::get_async_io_manager()
{
    return *m_async_io_manager.get();
}

std::filesystem::path engine::get_engine_asset_dir()
{
    return m_engine_asset_dir;
}

std::filesystem::path engine::get_game_asset_dir()
{
    return m_game_asset_dir;
}

std::filesystem::path engine::get_asset_cache_dir()
{
    return m_asset_cache_dir;
}

void engine::set_render_interface_type(ri_interface_type type)
{
    m_render_interface_type = type;
}

void engine::set_window_interface_type(window_interface_type type)
{
    m_window_interface_type = type;
}

void engine::set_input_interface_type(input_interface_type type)
{
    m_input_interface_type = type;
}

void engine::set_physics_interface_type(physics_interface_type type)
{
    m_physics_interface_type = type;
}

void engine::set_platform_interface_type(platform_interface_type type)
{
    m_platform_interface_type = type;
}

void engine::set_window_mode(const std::string& title, size_t width, size_t height, window_mode mode)
{
    m_window_title = title;
    m_window_width = width;
    m_window_height = height;
    m_window_mode = mode;

    if (m_window)
    {
        m_window->set_title(title);
        m_window->set_width(width);
        m_window->set_height(height);
        m_window->set_mode(mode);
        m_window->apply_changes();
    }
}


result<void> engine::register_asset_loaders(init_list& list)
{
    m_asset_manager->register_loader(std::make_unique<scene_loader>(*m_asset_manager, this));

    return true;
}

result<void> engine::unregister_asset_loaders()
{
    return true;
}

result<void> engine::create_task_scheduler(init_list& list)
{
    task_scheduler::init_state init_state;
    init_state.worker_count = std::thread::hardware_concurrency();

    // If you add new task queues, set up and appropriate weight here.
    static_assert(static_cast<int>(task_queue::COUNT) == 2);
    init_state.queue_weights[static_cast<int>(task_queue::standard)] = 1.0f;
    init_state.queue_weights[static_cast<int>(task_queue::loading)] = 0.75f; 

    db_log(engine, "Creating task scheduler with %zi workers.", init_state.worker_count);

    {
        memory_scope scope(memory_type::engine__task_scheduler, memory_scope::k_ignore_asset);
        m_task_scheduler = std::make_unique<task_scheduler>(init_state);
    }

    return true;
}

result<void> engine::destroy_task_scheduler()
{
    m_task_scheduler = nullptr;

    return true;
}

result<void> engine::create_filesystem(init_list& list)
{
    m_async_io_manager = async_io_manager::create();
    m_async_copy_manager = std::make_unique<async_copy_manager>();
    m_filesystem = std::make_unique<virtual_file_system>();

    std::filesystem::path appdata_path = get_special_path(special_path::app_data);
    m_saved_data_dir = appdata_path / "saved" / "users" / get_username();
    m_saved_machine_data_dir = appdata_path / "saved" / "machine";

    // Figure out what folders the engine and game assets are stored in.
    std::filesystem::path root_dir = get_application_path().parent_path();
    while (root_dir != root_dir.parent_path())
    {
        m_engine_asset_dir = root_dir / "engine" / "assets";
        m_game_asset_dir = root_dir / "games" / app::instance().get_name() / "assets";
        m_asset_cache_dir = root_dir / "intermediate" / "cache";
        m_thumbnail_asset_cache_dir = root_dir / "intermediate" / "thumbnails";
         
        if (std::filesystem::exists(m_engine_asset_dir) && 
            std::filesystem::exists(m_game_asset_dir))
        {
            break;
        }
        
        root_dir = root_dir.parent_path();
    }

    if (!std::filesystem::exists(m_engine_asset_dir))
    {
        db_fatal(engine, "Failed to find engine asset directory.");
        return false;
    }
    if (!std::filesystem::exists(m_game_asset_dir))
    {
        db_fatal(engine, "Failed to find game asset directory.");
        return false;
    }
    if (!std::filesystem::exists(m_asset_cache_dir))
    {
        if (!std::filesystem::create_directories(m_asset_cache_dir))
        {
            db_fatal(engine, "Failed to create asset cache directory: %s", m_asset_cache_dir.string().c_str());
            return false;
        }
    }
    if (!std::filesystem::exists(m_thumbnail_asset_cache_dir))
    {
        if (!std::filesystem::create_directories(m_thumbnail_asset_cache_dir))
        {
            db_fatal(engine, "Failed to create thumbnail asset cache directory: %s", m_thumbnail_asset_cache_dir.string().c_str());
            return false;
        }
    }
    if (!std::filesystem::exists(m_saved_data_dir))
    {
        if (!std::filesystem::create_directories(m_saved_data_dir))
        {
            db_fatal(engine, "Failed to create saved directory: %s", m_saved_data_dir.string().c_str());
            return false;
        }
    }
    if (!std::filesystem::exists(m_saved_machine_data_dir))
    {
        if (!std::filesystem::create_directories(m_saved_machine_data_dir))
        {
            db_fatal(engine, "Failed to create saved directory: %s", m_saved_machine_data_dir.string().c_str());
            return false;
        }
    }

    db_log(engine, "Engine asset directory: %s", m_engine_asset_dir.string().c_str());
    db_log(engine, "Game asset directory: %s", m_game_asset_dir.string().c_str());
    db_log(engine, "Asset cache directory: %s", m_asset_cache_dir.string().c_str());
    db_log(engine, "Save directory: %s", m_saved_data_dir.string().c_str());
    db_log(engine, "Machine save directory: %s", m_saved_machine_data_dir.string().c_str());

    // Create the main data protocol, engine and game assets are mounted to the same path, 
    // with game assets taking priority.
    m_filesystem->register_handler("data", 0, std::make_unique<virtual_file_system_disk_handler>(get_engine_asset_dir().string().c_str(), false));
    m_filesystem->register_handler("data", 1, std::make_unique<virtual_file_system_disk_handler>(get_game_asset_dir().string().c_str(), false));

    // Points to the local asset cache disk folder.
    m_filesystem->register_handler("local-cache", 0, std::make_unique<virtual_file_system_disk_handler>(get_asset_cache_dir().string().c_str(), false));

    // Points to the cache directory we store thumbnails of assets inside of.
    m_filesystem->register_handler("thumbnail-cache", 0, std::make_unique<virtual_file_system_disk_handler>(m_thumbnail_asset_cache_dir.string().c_str(), false));

    // The cache protocol redirects "clean" paths like cache:shaders/common.yaml to complex storage
    // locations, eg. local-cache:windows/1/4/6/1/146123421341_geometry_yaml.compiled
    // 
    // The primary purpose of this is just to make things simpler to debug as we use the "true"
    // asset paths rather than the mangled ones.
    // 
    // Paths to this protocol will be returned from things like the asset manager, you shouldn't try to 
    // construct paths within this protocol yourself as they will not work without the behind-the-scenes
    // remapping.
    m_filesystem->register_handler("cache", 0, std::make_unique<virtual_file_system_redirect_handler>(false));

    // The temporary mount is just a location on disk we can store temporary files in.
    m_filesystem->register_handler("temp", 0, std::make_unique<virtual_file_system_disk_handler>(std::filesystem::temp_directory_path().string().c_str(), false));

    // This handler is used to directly access the host file system.
    m_filesystem->register_handler("host", 0, std::make_unique<virtual_file_system_disk_handler>("", false));

    // This handler is used to store saved data.
    m_filesystem->register_handler("save", 0, std::make_unique<virtual_file_system_disk_handler>(m_saved_data_dir.string().c_str(), false));

    // This handler is used to store saved data that is machine specific.
    m_filesystem->register_handler("machine-save", 0, std::make_unique<virtual_file_system_disk_handler>(m_saved_machine_data_dir.string().c_str(), false));

    return true;
}

result<void> engine::destroy_filesystem()
{
    m_filesystem = nullptr;
    m_async_io_manager = nullptr;

    return true;
}

result<void> engine::create_statistics_manager(init_list& list)
{
    m_statistics = std::make_unique<statistics_manager>();

    m_stats_frame_time_game = m_statistics->find_or_create_channel("frame time/game", 1.0f);
    m_stats_frame_rate = m_statistics->find_or_create_channel("frame rate");

    return true;
}

result<void> engine::destroy_statistics_manager()
{
    m_statistics = nullptr;

    return true;
}

result<void> engine::create_asset_manager(init_list& list)
{
    m_asset_manager = std::make_unique<asset_manager>(get_platform(), get_config());
    m_asset_manager->register_cache(std::make_unique<asset_cache_disk>("local-cache", "cache", false));

    m_asset_database = std::make_unique<asset_database>(m_asset_manager.get());

    return true;
}

result<void> engine::destroy_asset_manager()
{
    m_asset_database = nullptr;
    m_asset_manager = nullptr;

    return true;
}

result<void> engine::create_window_interface(init_list& list)
{
    switch (m_window_interface_type)
    {
    case window_interface_type::sdl:
        {
            m_window_interface = std::make_unique<sdl_window_interface>(m_platform_interface.get());
            m_window_interface->register_init(list);
            break;
        }
    default:
        {
            db_error(core, "Windowing type requested is not implemented.");
            return standard_errors::no_implementation;
        }
    }

    return true;
}

result<void> engine::destroy_window_interface()
{
    m_window_interface = nullptr;

    return true;
}

result<void> engine::create_input_interface(init_list& list)
{
    switch (m_input_interface_type)
    {
    case input_interface_type::sdl:
        {
            m_input_interface = std::make_unique<sdl_input_interface>(m_platform_interface.get(), m_window.get());
            m_input_interface->register_init(list);
            break;
        }
    default:
        {
            db_error(core, "Input interface type requested is not implemented.");
            return standard_errors::no_implementation;
        }
    }

    return true;
}

result<void> engine::destroy_input_interface()
{
    m_input_interface = nullptr;

    return true;
}

result<void> engine::create_physics_interface(init_list& list)
{
    switch (m_physics_interface_type)
    {
    case physics_interface_type::jolt:
        {
            m_physics_interface = std::make_unique<jolt_physics_interface>();
            m_physics_interface->register_init(list);
            break;
        }
    default:
        {
            db_error(core, "Physics interface type requested is not implemented.");
            return standard_errors::no_implementation;
        }
    }

    return true;
}

result<void> engine::destroy_physics_interface()
{
    m_physics_interface = nullptr;

    return true;
}

result<void> engine::create_platform_interface(init_list& list)
{
    switch (m_platform_interface_type)
    {
    case platform_interface_type::sdl:
        {
            m_platform_interface = std::make_unique<sdl_platform_interface>();
            m_platform_interface->register_init(list);
            break;
        }
    default:
        {
            db_error(core, "Platform interface type requested is not implemented.");
            return standard_errors::no_implementation;
        }
    }

    return true;
}

result<void> engine::destroy_platform_interface()
{
    m_platform_interface = nullptr;

    return true;
}

result<void> engine::create_render_interface(init_list& list)
{
    switch (m_render_interface_type)
    {
#ifdef WS_WINDOWS
    case ri_interface_type::dx12:
        {
            m_render_interface = std::make_unique<dx12_render_interface>((size_t)ray_type::COUNT, (size_t)material_domain::COUNT);
            m_render_interface->register_init(list);
            break;
        }
#endif
    default:
        {
            db_error(core, "Renderer type requested is not implemented.");
            return standard_errors::no_implementation;
        }
    }

    m_asset_database->set_render_interface(m_render_interface.get());

    return true;
}

result<void> engine::destroy_render_interface()
{
    m_render_interface = nullptr;

    return true;
}

result<void> engine::create_renderer(init_list& list)
{
    m_renderer = std::make_unique<renderer>(
        *m_render_interface.get(), 
        *m_input_interface.get(), 
        *m_window.get(), 
        *m_asset_manager.get());
    m_renderer->register_init(list);

    return true;
}

result<void> engine::destroy_renderer()
{
    m_renderer = nullptr;

    return true;
}

result<void> engine::create_main_window(init_list& list)
{
    m_window = m_window_interface->create_window(m_window_title.c_str(), m_window_width, m_window_height, m_window_mode, m_render_interface_type);
    if (m_window == nullptr)
    {
        db_error(core, "Failed to create main window.");
        return standard_errors::failed;
    }

    return true;
}

result<void> engine::destroy_main_window()
{
    m_window = nullptr;
    return true;
}

result<void> engine::load_config(init_list& list)
{
    std::string engine_config = "data:engine.config";
    std::string game_config = "data:game.config";
    std::string user_save_config = "save:cvars_user.yaml";
    std::string machine_save_config = "machine-save:cvars_machine.yaml";

    // Set all cvars that are used for configuring settings.
    //cvar_gpu_memory_mb->set_int(1024);
    //cvar_platform->set_string("windows");


    cvar_manager& manager = cvar_manager::get();
    if (!manager.add_config_file(engine_config.c_str()) ||
        !manager.add_config_file(game_config.c_str()))
    {
        return false;
    }
    manager.set_save_path(user_save_config.c_str(), machine_save_config.c_str());
    manager.load();
    manager.evaluate();
    if (!manager.save())
    {
        return false;
    }

    return true;
}

result<void> engine::create_default_world(init_list& list)
{
    m_default_world = create_world("Default World");
    return true;
}

result<void> engine::destroy_default_world()
{
    destroy_world(m_default_world);
    return true;
}

void engine::set_default_world(world* new_world)
{
    if (m_default_world)
    {
        destroy_world(m_default_world);
    }
    m_default_world = new_world;
}

void engine::load_world(const char* path)
{
    db_log(engine, "Requesting load of world: %s", path);

    m_loading_world = m_asset_manager->request_asset<scene>(path, 0);
    m_loading_world_start_time = get_seconds();
}

bool engine::is_loading_world()
{
    return m_loading_world.is_valid();
}

result<void> engine::create_presenter(init_list& list)
{
    m_presenter = std::make_unique<presenter>(*this);
    m_presenter->register_init(list);

    return true;
}

result<void> engine::destroy_presenter()
{
    m_presenter = nullptr;

    return true;
}

result<void> engine::create_editor(init_list& list)
{
    m_editor = std::make_unique<editor>(*this);
    m_editor->register_init(list);

    return true;
}

result<void> engine::destroy_editor()
{
    m_editor = nullptr;

    return true;
}

std::vector<world*> engine::get_worlds()
{
    std::vector<world*> result;
    for (auto& world : m_worlds)
    {
        result.push_back(world.get());
    }
    return result;
}

world& engine::get_default_world()
{
    return *m_default_world;
}

world* engine::create_world(const char* name)
{
    db_log(engine, "Creating new world: %s", name);

    std::unique_ptr<world> new_world = std::make_unique<world>(*this);
    world* result = new_world.get();
    m_worlds.push_back(std::move(new_world));

    if (m_system_registration_callback)
    {
        m_system_registration_callback(result->get_object_manager());
    }

    return result;
}

void engine::destroy_world(world* world)
{
    auto iter = std::find_if(m_worlds.begin(), m_worlds.end(), [world](auto& existing_world) {
        return existing_world.get() == world;    
    });
    if (iter == m_worlds.end())
    {
        return;
    }

    db_log(engine, "Destroying world: %s", world->get_name());
    m_worlds.erase(iter);
}

void engine::set_system_registration_callback(system_registration_callback_t callback)
{
    m_system_registration_callback = callback;
}

bool engine::get_mouse_over_viewport()
{
    return m_mouse_over_viewport;
}

void engine::set_mouse_over_viewport(bool over_viewport)
{
    m_mouse_over_viewport = over_viewport;
}

}; // namespace ws
