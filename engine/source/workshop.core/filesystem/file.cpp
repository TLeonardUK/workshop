// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/file.h"
#include "workshop.core/containers/string.h"

#include <filesystem>
#include <array>
#include <fstream>
#include <sstream>

namespace {

std::filesystem::path g_application_path;
std::vector<std::string> g_command_line;
std::array<std::filesystem::path, static_cast<int>(ws::special_path::count)> g_special_paths;

};

namespace ws {

std::filesystem::path get_special_path(special_path path)
{
    return g_special_paths[static_cast<int>(path)];
}

void set_special_path(special_path path, const std::filesystem::path& physical_path)
{
    if (!std::filesystem::exists(physical_path))
    {
        std::filesystem::create_directories(physical_path);
    }

    g_special_paths[static_cast<int>(path)] = physical_path;
}

std::filesystem::path get_application_path()
{
    return g_application_path;
}

std::vector<std::string> get_command_line()
{
    return g_command_line;
}

bool is_option_set(const char* name)
{ 
    // TODO: Parse this stuff better.
    std::string option_1 = "-" + std::string(name);
    std::string option_2 = "--" + std::string(name);

    for (std::string& value : g_command_line)
    {
        if (value == option_1 || value == option_2)
        {
            return true;
        }
    }
    return false;
}

void set_command_line(const std::vector<std::string>& args)
{
    db_assert(args.size() > 0);

    g_command_line = std::vector<std::string>(args.begin() + 1, args.end());
    g_application_path = args[0];
}

char get_drive_letter(const std::filesystem::path& path)
{
    if (!path.has_root_name())
    {
        return '\0';
    }
    return path.root_name().string().c_str()[0];
}

result<std::filesystem::path> get_relative_path(const std::filesystem::path& path, const std::filesystem::path& root)
{
    // Ho boy this is a shit way to check if the path is relative.
    if (string_starts_with(string_lower(path.string()), string_lower(root.string())))
    {
        std::string relative = path.string().substr(root.string().size() + 1);
        return std::filesystem::path(relative);
    }
    return standard_errors::path_not_relative;
}

result<void> read_all_text(const std::filesystem::path& path, std::string& Output)
{
    std::ifstream file(path.c_str());
    if (!file.is_open())
    {
        return standard_errors::open_file_failed;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    Output = buffer.str();

    file.close();

    return true;
}

result<void> write_all_text(const std::filesystem::path& path, const std::string& Input)
{
    std::ofstream file(path.c_str());
    if (!file.is_open())
    {
        return standard_errors::open_file_failed;
    }

    file << Input;

    file.close();

    return true;
}

result<void> read_all_bytes(const std::filesystem::path& path, std::vector<uint8_t>& Output)
{
    FILE* file = fopen(path.string().c_str(), "rb");
    if (file == nullptr)
    {
        return standard_errors::open_file_failed;
    }

    fseek(file, 0, SEEK_END);
    Output.resize(ftell(file));
    fseek(file, 0, SEEK_SET);

    size_t bytesRead = 0;
    while (bytesRead < Output.size())
    {
        bytesRead += fread((char*)Output.data() + bytesRead, 1, Output.size() - bytesRead, file);
    }

    fclose(file);

    return true;
}

result<void> write_all_bytes(const std::filesystem::path& path, const std::vector<uint8_t>& Input)
{
    FILE* file = fopen(path.string().c_str(), "wb");
    if (file == nullptr)
    {
        return standard_errors::open_file_failed;
    }

    size_t bytesWritten = 0;
    while (bytesWritten < Input.size())
    {
        bytesWritten += fwrite((char*)Input.data() + bytesWritten, 1, Input.size() - bytesWritten, file);
    }

    fclose(file);

    return true;
}

}; // namespace workshop
