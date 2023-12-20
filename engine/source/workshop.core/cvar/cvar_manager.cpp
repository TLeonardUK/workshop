// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/cvar/cvar_manager.h"
#include "workshop.core/cvar/cvar.h"
#include "workshop.core/filesystem/file.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/containers/string.h"

#include <string>

#pragma optimize("", off)

namespace ws {

namespace {

constexpr const char* k_cvar_save_descriptor_type = "cvar_save";
constexpr size_t k_cvar_save_descriptor_minimum_version = 1;
constexpr size_t k_cvar_save_descriptor_current_version = 1;

};

cvar<int> cvar_test_machine(cvar_flag::saved|cvar_flag::machine_specific, 0, "test_machine", "test a");
cvar<int> cvar_test_user(cvar_flag::saved, 0, "test_user", "test a");

cvar_base* cvar_manager::find_cvar(const char* name)
{
    std::scoped_lock lock(m_mutex);
    for (cvar_base* base : m_cvars)
    {
        if (_stricmp(base->get_name(), name) == 0)
        {
            return base;
        }
    }
    return nullptr;
}

std::vector<cvar_base*> cvar_manager::get_cvars()
{
    std::scoped_lock lock(m_mutex);
    return m_cvars;
}

void cvar_manager::register_cvar(cvar_base* value)
{
    std::scoped_lock lock(m_mutex);
    m_cvars.push_back(value);
}

void cvar_manager::unregister_cvar(cvar_base* value)
{
    std::scoped_lock lock(m_mutex);
    std::erase(m_cvars, value);
}

void cvar_manager::set_save_path(const char* user_path, const char* machine_path)
{
    std::scoped_lock lock(m_mutex);

    m_user_save_path = user_path;
    m_machine_save_path = machine_path;
}

bool cvar_manager::save()
{
    if (!save_filtered(m_machine_save_path.c_str(), true) ||
        !save_filtered(m_user_save_path.c_str(), false))
    {
        return false;
    }
    return true;
}

bool cvar_manager::save_filtered(const char* path, bool machine_specific)
{
    std::scoped_lock lock(m_mutex);

    std::string output = "";

    output += "# ================================================================================================\n";
    output += "#  workshop\n";
    output += "#  Copyright (C) 2023 Tim Leonard\n";
    output += "# ================================================================================================\n";
    output += string_format("type: %s\n", k_cvar_save_descriptor_type);
    output += string_format("version: %i\n", k_cvar_save_descriptor_current_version);
    output += "\n";
    output += "values:\n";

    bool has_saved_values = false;
    
    for (cvar_base* base : m_cvars)
    {
        if (!base->has_flag(cvar_flag::saved))
        {
            continue;
        }
        if (base->has_flag(cvar_flag::machine_specific) != machine_specific)
        {
            continue;
        }

        if (base->get_value_type() == typeid(int))
        {
            output += string_format("  %s: %i\n", base->get_name(), base->get_int());
            has_saved_values = true;
        }
        else if (base->get_value_type() == typeid(std::string))
        {
            output += string_format("  %s: %s\n", base->get_name(), base->get_string());
            has_saved_values = true;
        }
        else if (base->get_value_type() == typeid(float))
        {
            output += string_format("  %s: %.8f\n", base->get_name(), base->get_float());
            has_saved_values = true;
        }
        else if (base->get_value_type() == typeid(bool))
        {
            output += string_format("  %s: %i\n", base->get_name(), base->get_bool() ? 1 : 0);
            has_saved_values = true;
        }
    }

    if (!has_saved_values)
    {
        output += "  -\n";
    }

    std::unique_ptr<stream> stream = virtual_file_system::get().open(path, true);
    if (!stream)
    {
        return false;
    }

    stream->write(output.c_str(), output.size());

    return true;
}

bool cvar_manager::load_descriptor(const char* path, YAML::Node& node)
{
    std::unique_ptr<stream> stream = virtual_file_system::get().open(path, false);
    if (!stream)
    {
        return false;
    }

    // Read in the asset header and check version and type is correct.
    std::string contents = stream->read_all_string();
    try
    {
        node = YAML::Load(contents);

        YAML::Node type_node = node["type"];
        YAML::Node version_node = node["version"];

        if (!type_node.IsDefined())
        {
            throw std::exception("type node is not defined.");
        }
        if (type_node.Type() != YAML::NodeType::Scalar)
        {
            throw std::exception("type node is the wrong type, expected a string.");
        }

        if (!version_node.IsDefined())
        {
            throw std::exception("version node is not defined.");
        }
        if (version_node.Type() != YAML::NodeType::Scalar)
        {
            throw std::exception("version node is the wrong type, expected a string.");
        }

        std::string type = type_node.as<std::string>();
        size_t version = version_node.as<size_t>();

        if (type != k_cvar_save_descriptor_type)
        {
            throw std::exception(string_format("Type '%s' is not of expected type '%s'.", type.c_str(), k_cvar_save_descriptor_type).c_str());
        }

        if (version < k_cvar_save_descriptor_minimum_version)
        {
            throw std::exception(string_format("Version '%zi' is older than the minimum supported '%zi'.", version, k_cvar_save_descriptor_minimum_version).c_str());
        }
        if (version > k_cvar_save_descriptor_current_version)
        {
            throw std::exception(string_format("Version '%zi' is newer than the maximum supported '%zi'.", version, k_cvar_save_descriptor_current_version).c_str());
        }
    }
    catch (YAML::ParserException& exception)
    {
        db_error(core, "[%s] Error parsing cvar save file: %s", path, exception.msg.c_str());
        return false;
    }
    catch (std::exception& exception)
    {
        db_error(core, "[%s] Error loading cvar save file: %s", path, exception.what());
        return false;
    }

    return true;
}

bool cvar_manager::load_filtered(const char* path, bool machine_specific)
{
    std::scoped_lock lock(m_mutex);

    YAML::Node node;

    // Load in file and validate version/type.
    if (!load_descriptor(path, node))
    {
        return false;
    }

    // Read in all cvar values.
    YAML::Node values_node = node["values"];
    if (!values_node.IsDefined())
    {
        return true;
    }
    if (values_node.Type() != YAML::NodeType::Map)
    {
        db_error(core, "[%s] values node is invalid data type.", path);
        return false;
    }

    for (auto iter = values_node.begin(); iter != values_node.end(); iter++)
    {
        if (!iter->second.IsScalar())
        {
            db_error(core, "[%s] cvar value was not scalar value.", path);
            return false;
        }
        else
        {
            std::string name = iter->first.as<std::string>();
            std::string value = iter->second.as<std::string>();

            cvar_base* instance = find_cvar(name.c_str());
            if (!instance)
            {
                db_warning(core, "[%s] cvar '%s' from save file was not found, ignoring.", path, name.c_str());
                continue;
            }

            if (instance->get_value_type() == typeid(int))
            {
                instance->set_int(atoi(value.c_str()), cvar_source::set_by_save);
            }
            else if (instance->get_value_type() == typeid(std::string))
            {
                instance->set_string(value.c_str(), cvar_source::set_by_save);
            }
            else if (instance->get_value_type() == typeid(float))
            {
                instance->set_float((float)atof(value.c_str()), cvar_source::set_by_save);
            }
            else if (instance->get_value_type() == typeid(bool))
            {
                bool is_true = (value == "1" || _stricmp(value.c_str(), "true") == 0);
                instance->set_bool(is_true, cvar_source::set_by_save);
            }
        }
    }

    return true;
}

bool cvar_manager::load()
{
    if (!load_filtered(m_machine_save_path.c_str(), true) ||
        !load_filtered(m_user_save_path.c_str(), false))
    {
        return false;
    }
    return true;
}

bool cvar_manager::add_config_file(const char* path)
{
    std::scoped_lock lock(m_mutex);

    m_config_paths.push_back(path);

    // TODO: Load and parse the file.

    return true;
}

void cvar_manager::evaluate()
{
    std::scoped_lock lock(m_mutex);

    // TODO: Evaluate the config properties.
}

}; // namespace ws
