// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/debug/debug.h"

#include <filesystem>

namespace ws {

// ================================================================================================
//  Special paths defined for specific use cases.
// ================================================================================================
enum class special_path
{
    // Stores data common between multiple workshop apps.
    common_data,

    // Stores data specific to the running app.
    app_data,

    // Stores logging files specific to the running app.
    app_logs,

    count
};

// ================================================================================================
//  Gets the path to the exe/dll/etc thats currently executing.
// ================================================================================================
std::filesystem::path get_application_path();

// ================================================================================================
//  Gets a special path. 
// ================================================================================================
std::filesystem::path get_special_path(special_path path);

// ================================================================================================
//  Sets a special path. Also creates it if it doesn't already exist.
//  Shouldn't normally be used by anything but the entry point. 
// ================================================================================================
void set_special_path(special_path path, const std::filesystem::path& physical_path);

// ================================================================================================
//  Gets the command line arguments passed to the application.
//  Arguments DO NOT include first argument that contains the path to the application.
// ================================================================================================
std::vector<std::string> get_command_line();

// ================================================================================================
//  Sets the command line arguments passed to the application. Shouldn't normally be used
//  by anything but the entry point. 
//  Arguments should include first argument that contains the path to the application.
// ================================================================================================
void set_command_line(const std::vector<std::string>& args);

// ================================================================================================
//  Gets if a command line option is set.
// ================================================================================================
bool is_option_set(const char* name);

// ================================================================================================
//  Gets the directory non-roadming application data should be stored in.
// ================================================================================================
std::filesystem::path get_local_appdata_directory();

// ================================================================================================
//  Gets the drive letter from the given path. If path is relative or not
//  drive rooted then null is returned.
// ================================================================================================
char get_drive_letter(const std::filesystem::path& path);

// ================================================================================================
//  Strips root off path and returns it. If path is not contained in root an error is returned.
// ================================================================================================
result<std::filesystem::path> get_relative_path(const std::filesystem::path& path, const std::filesystem::path& root);

// ================================================================================================
//  Reads contents of a file into a string.
// ================================================================================================
result<void> read_all_text(const std::filesystem::path& path, std::string& Output);

// ================================================================================================
//  Writes contents of a string into a file.
// ================================================================================================
result<void> write_all_text(const std::filesystem::path& path, const std::string& Input);

// ================================================================================================
//  Reads contents of a file into a vector.
// ================================================================================================
result<void> read_all_bytes(const std::filesystem::path& path, std::vector<uint8_t>& Output);

// ================================================================================================
//  Writes contents of a vector into a file.
// ================================================================================================
result<void> write_all_bytes(const std::filesystem::path& path, const std::vector<uint8_t>& Input);

}; // namespace workshop
