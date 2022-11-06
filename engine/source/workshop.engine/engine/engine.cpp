// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.render_interface/render_interface.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_effect.h"
#include "workshop.windowing/windowing.h"
#include "workshop.core/utils/init_list.h"
#include "workshop.core/perf/profile.h"
#include "workshop.core/async/task_scheduler.h"
#include "workshop.core/async/async.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/filesystem/virtual_file_system_disk_handler.h"
#include "workshop.core/filesystem/virtual_file_system_aliased_disk_handler.h"
#include "workshop.core/filesystem/file.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/app/app.h"

#include "workshop.engine/assets/asset_manager.h"
#include "workshop.engine/assets/caches/asset_cache_disk.h"
#include "workshop.engine/assets/types/shader/shader.h"
#include "workshop.engine/assets/types/shader/shader_loader.h"

#include "workshop.windowing.sdl/sdl_windowing.h"

#ifdef WS_WINDOWS
#include "workshop.render_interface.dx12/dx12_render_interface.h"
#endif

namespace ws {

engine::engine() = default;
engine::~engine() = default;

void engine::step()
{
    profile_marker(profile_colors::engine, "frame %zi", (size_t)m_frame_time.frame_count);
    profile_variable(m_frame_time.delta_seconds, "delta seconds");

    m_frame_time.step();
    m_presenter->step(m_frame_time);
    m_windowing->pump_events();
}

void engine::register_init(init_list& list)
{
    list.add_step(
        "Task Scheduler",
        [this, &list]() -> result<void> { return create_task_scheduler(list); },
        [this, &list]() -> result<void> { return destroy_task_scheduler(); }
    );
    list.add_step(
        "Filesystem",
        [this, &list]() -> result<void> { return create_filesystem(list); },
        [this, &list]() -> result<void> { return destroy_filesystem(); }
    );
    list.add_step(
        "Asset Manager",
        [this, &list]() -> result<void> { return create_asset_manager(list); },
        [this, &list]() -> result<void> { return destroy_asset_manager(); }
    );
    list.add_step(
        "Windowing",
        [this, &list]() -> result<void> { return create_windowing(list); },
        [this, &list]() -> result<void> { return destroy_windowing(); }
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
        "Renderer",
        [this, &list]() -> result<void> { return create_renderer(list); },
        [this, &list]() -> result<void> { return destroy_renderer(); }
    );
    list.add_step(
        "Presentation",
        [this, &list]() -> result<void> { return create_presenter(list); },
        [this, &list]() -> result<void> { return destroy_presenter(); }
    );
}

render_interface& engine::get_render_interface()
{
    return *m_render_interface.get();
}

renderer& engine::get_renderer()
{
    return *m_renderer.get();
}

windowing& engine::get_windowing()
{
    return *m_windowing.get();
}

window& engine::get_main_window()
{
    return *m_window.get();
}

virtual_file_system& engine::get_filesystem()
{
    return *m_filesystem.get();
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

void engine::set_render_interface_type(render_interface_type type)
{
    m_render_interface_type = type;
}

void engine::set_windowing_type(windowing_type type)
{
    m_windowing_type = type;
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

result<void> engine::create_task_scheduler(init_list& list)
{
    task_scheduler::init_state init_state;
    init_state.worker_count = std::thread::hardware_concurrency();

    // If you add new task queues, set up and appropriate weight here.
    static_assert(static_cast<int>(task_queue::COUNT) == 2);
    init_state.queue_weights[static_cast<int>(task_queue::standard)] = 1.0f;
    init_state.queue_weights[static_cast<int>(task_queue::loading)] = 0.5f;

    db_log(engine, "Creating task scheduler with %zi workers.", init_state.worker_count);

    m_task_scheduler = std::make_unique<task_scheduler>(init_state);

    return true;
}

result<void> engine::destroy_task_scheduler()
{
    m_task_scheduler = nullptr;

    return true;
}

result<void> engine::create_filesystem(init_list& list)
{
    m_filesystem = std::make_unique<virtual_file_system>();

    // Figure out what folders the engine and game assets are stored in.
    std::filesystem::path root_dir = get_application_path().parent_path();
    while (root_dir != root_dir.parent_path())
    {
        m_engine_asset_dir = root_dir / "engine" / "assets";
        m_game_asset_dir = root_dir / "games" / app::instance().get_name() / "assets";
        m_asset_cache_dir = root_dir / "intermediate" / "cache";
         
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

    db_log(engine, "Engine asset directory: %s", m_engine_asset_dir.string().c_str());
    db_log(engine, "Game asset directory: %s", m_game_asset_dir.string().c_str());
    db_log(engine, "Asset cache directory: %s", m_asset_cache_dir.string().c_str());

    // Create the main data protocol, engine and game assets are mounted to the same path, 
    // with game assets taking priority.
    m_filesystem->register_handler("data", 0, std::make_unique<virtual_file_system_disk_handler>(get_engine_asset_dir().string().c_str(), true));
    m_filesystem->register_handler("data", 1, std::make_unique<virtual_file_system_disk_handler>(get_game_asset_dir().string().c_str(), true));

    // The cache protocol loads from the compiled asset cache. You should not try and query this
    // protocol directly, you should load from the data protocol, this protocol handler is lazily
    // filled as assets are requested.
    m_filesystem->register_handler("cache", 0, std::make_unique<virtual_file_system_aliased_disk_handler>(false));

    // The temporary mount is just a location on disk we can store temporary files in - for things like
    // intermediate compiled files.
    m_filesystem->register_handler("temp", 0, std::make_unique<virtual_file_system_disk_handler>(std::filesystem::temp_directory_path().string().c_str(), false));

    return true;
}

result<void> engine::destroy_filesystem()
{
    m_filesystem = nullptr;

    return true;
}


result<void> engine::create_asset_manager(init_list& list)
{
    m_asset_manager = std::make_unique<asset_manager>(get_platform(), get_config());
    m_asset_manager->register_loader(std::make_unique<shader_loader>(*this));
    m_asset_manager->register_cache(std::make_unique<asset_cache_disk>("cache", m_asset_cache_dir, false));

    return true;
}

result<void> engine::destroy_asset_manager()
{
    m_asset_manager = nullptr;

    return true;
}

result<void> engine::create_windowing(init_list& list)
{
    switch (m_windowing_type)
    {
    case windowing_type::sdl:
        {
            m_windowing = std::make_unique<sdl_windowing>();
            m_windowing->register_init(list);
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

result<void> engine::destroy_windowing()
{
    m_windowing = nullptr;

    return true;
}

result<void> engine::create_render_interface(init_list& list)
{
    switch (m_render_interface_type)
    {
#ifdef WS_WINDOWS
    case render_interface_type::dx12:
        {
            m_render_interface = std::make_unique<dx12_render_interface>();
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

    return true;
}

result<void> engine::destroy_render_interface()
{
    m_render_interface = nullptr;

    return true;
}

result<void> engine::create_renderer(init_list& list)
{
    m_renderer = std::make_unique<renderer>(*m_render_interface.get(), *m_window.get());
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
    m_window = m_windowing->create_window(m_window_title.c_str(), m_window_width, m_window_height, m_window_mode, m_render_interface_type);
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

result<void> engine::create_presenter(init_list& list)
{
    m_presenter = std::make_unique<presenter>(*this);
    m_presenter->register_init(list);


    asset_ptr<shader> instance = m_asset_manager->request_asset<shader>("data:shaders/geometry.yaml", 0);
    instance.wait_for_load();


    return true;
}

result<void> engine::destroy_presenter()
{
    m_presenter = nullptr;

    return true;
}

}; // namespace ws
