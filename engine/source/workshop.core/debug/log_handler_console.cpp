// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/debug/log_handler_console.h"
#include "workshop.core/debug/debug.h"

#include <array>

namespace ws {

void log_handler_console::write(log_level level, const std::string& message)
{
    static std::array<console_color, static_cast<int>(console_color::count)> level_colors = {
        console_color::red,     // fatal,
        console_color::red,     // error,
        console_color::yellow,  // warning,
        console_color::green,   // success,
        console_color::grey,    // log,
        console_color::grey,    // verbose,
    };

    db_console_write(message.c_str(), level_colors[static_cast<int>(level)]);
}

}; // namespace workshop
