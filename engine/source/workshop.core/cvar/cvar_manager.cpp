// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/cvar/cvar_manager.h"
#include "workshop.core/cvar/cvar.h"
#include "workshop.core/cvar/cvar_config_file.h"
#include "workshop.core/filesystem/file.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/containers/string.h"
#include "workshop.core/utils/time.h"

#include <string>

// TODO: 
//      Add setting cvar's from command line
//      Could use the config files to + command lines to add debug functionality?

namespace ws {

namespace {

constexpr const char* k_cvar_save_descriptor_type = "cvar_save";
constexpr size_t k_cvar_save_descriptor_minimum_version = 1;
constexpr size_t k_cvar_save_descriptor_current_version = 1;

};

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
    if (std::find(m_cvars.begin(), m_cvars.end(), value) != m_cvars.end())
    {
        return;
    }
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

    bool has_saved_values = false;
    std::string value_output = "";
    value_output += "values:\n";

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
            value_output += string_format("  %s: %i\n", base->get_name(), base->get_int());
            has_saved_values = true;
        }
        else if (base->get_value_type() == typeid(std::string))
        {
            value_output += string_format("  %s: %s\n", base->get_name(), base->get_string());
            has_saved_values = true;
        }
        else if (base->get_value_type() == typeid(float))
        {
            value_output += string_format("  %s: %.8f\n", base->get_name(), base->get_float());
            has_saved_values = true;
        }
        else if (base->get_value_type() == typeid(bool))
        {
            value_output += string_format("  %s: %i\n", base->get_name(), base->get_bool() ? 1 : 0);
            has_saved_values = true;
        }
    }

    if (has_saved_values)
    {
        output += value_output;
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

            instance->coerce_from_string(value.c_str(), cvar_source::set_by_save);
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

    db_log(core, "Parsing cvar config file: %s", path);
#if 0
    double start = get_seconds();
#endif

    std::unique_ptr<cvar_config_file> config = std::make_unique<cvar_config_file>();
    if (!config->parse(path))
    {
        return false;
    }

#if 0
    double elapsed = get_seconds() - start;
    db_log(core, "Parsed in %.2f ms", elapsed * 1000.0f);
#endif

    m_config_files.push_back(std::move(config));
    return true;
}

void cvar_manager::evaluate()
{
    std::scoped_lock lock(m_mutex);

    db_log(core, "Evaluating cvar config files ...");
#if 0
    double start = get_seconds();
#endif

    for (auto& ptr : m_config_files)
    {
        ptr->evaluate_defaults();
    }

    for (auto& ptr : m_config_files)
    {
        ptr->evaluate();
}

#if 0
    double elapsed = get_seconds() - start;
    db_log(core, "Evaluated in %.2f ms", elapsed * 1000.0f);
#endif
}

void cvar_manager::reset_to_default()
{
    std::scoped_lock lock(m_mutex);

    for (cvar_base* base : m_cvars)
    {
        base->reset_to_default();
    }

    evaluate();
}

}; // namespace ws
