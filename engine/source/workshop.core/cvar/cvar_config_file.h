// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/cvar/cvar_config_lexer.h"
#include "workshop.core/cvar/cvar_config_parser.h"

#include <vector>
#include <memory>

namespace ws {

// Responsible for parsing and storing the contents of a configuration file.
class cvar_config_file
{
public:
    // Parses the file from disk and returns true if successful.
    bool parse(const char* path);

    // Evaluates the default assignments in the configuration file.
    bool evaluate_defaults();

    // Evaluates the assignments in the configuration file.
    bool evaluate();

private:
    std::vector<std::unique_ptr<cvar_config_ast_node>> m_ast_nodes;

};

}; // namespace ws
