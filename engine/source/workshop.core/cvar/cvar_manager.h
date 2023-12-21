// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/singleton.h"
#include "thirdparty/yamlcpp/include/yaml-cpp/yaml.h"
#include "workshop.core/cvar/cvar_config_file.h"

#include <memory>
#include <filesystem>

namespace ws {

class cvar_base;
class cvar_config_file;

// Holds references to all cvars created by the game.
class cvar_manager 
    : public auto_create_singleton<cvar_manager>
{
public:
    
    // Adds a configuration file that cvar default values will be loaded from.
    bool add_config_file(const char* path);

    // Sets the location on disk that the cvar state will be saved to
    void set_save_path(const char* user_path, const char* machine_path);

    // Saves the state of all saved cvar's to disk.
    bool save();

    // Loads the state of all saved cvar's from disk. 
    bool load();

    // Runs the config files added by add_config_file and applies any 
    // setting changes.
    void evaluate();

    // Resets all cvars to their code default values.
    void reset_to_default();

    // Finds a cvar with the given name or returns null if not found.
    // The search is case-insensitive.
    cvar_base* find_cvar(const char* name);
    
    // Gets a list of all registered cvars.
    std::vector<cvar_base*> get_cvars();

private:
    friend class cvar_base;

    // Loads the header descriptor from a save file and validates version/type.
    bool load_descriptor(const char* path, YAML::Node& node);

    // Saves cvars that are either machine specific or not to a file.
    bool save_filtered(const char* path, bool machine_specific);

    // Loads cvars from either a machine or non-machine specific save file.
    bool load_filtered(const char* path, bool machine_specific);

    // Register/unregister cvar's, called from the constructor/destructor of cvar_base.
    void register_cvar(cvar_base* value);
    void unregister_cvar(cvar_base* value);

private:

    std::recursive_mutex m_mutex;
    std::vector<cvar_base*> m_cvars;

    std::string m_user_save_path;
    std::string m_machine_save_path;
    std::vector<std::string> m_config_paths;

    std::vector<std::unique_ptr<cvar_config_file>> m_config_files;

};

}; // namespace ws
