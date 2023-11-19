// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/debug/log_handler_console.h"
#include "workshop.core/debug/log_handler_file.h"
#include "workshop.core/utils/version.h"
#include "workshop.core/filesystem/file.h"
#include "workshop.core/perf/profile.h"

#include "workshop.core/memory/memory.h"
#include "workshop.core/memory/memory_tracker.h"

#include "workshop.core/app/app.h"

#include <filesystem>

extern std::shared_ptr<ws::app> make_app();

namespace ws {

int entry_point(int argc, char* argv[])
{
    db_set_thread_name("Main Thread");

    // Store command line arguments for later use.
    std::vector<std::string> command_line;
    command_line.resize(argc);

    for (int i = 0; i < argc; i++)
    {
        command_line[i] = argv[i];
    }

    set_command_line(command_line);

    // Setup application we are running.
    std::shared_ptr<app> app = make_app();

    // Setup default logging handlers.
    set_special_path(special_path::common_data, ws::get_local_appdata_directory() / "workshop" / "common");
    set_special_path(special_path::app_data,    ws::get_local_appdata_directory() / "workshop" / app->get_name());
    set_special_path(special_path::app_logs,    ws::get_local_appdata_directory() / "workshop" / app->get_name() / "logs");
    
    log_handler_console console_logger;
    log_handler_file file_logger(get_special_path(special_path::app_logs), 5, 16 * 1024 * 1024);

    version_info version = get_version();

    db_log(core, "Workshop: %s", app->get_name().c_str());
    db_log(core, "Version %s", version.string.c_str());
    db_log(core, "");

    // Initialize the profiling library.
    platform_perf_init();

    // Call the applications entry point.
    if (result<void> ret = app->run(); !ret)
    {
        return static_cast<int>(ret.get_error());
    }

    return 0;
}

}; // namespace workshop
