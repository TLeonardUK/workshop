// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/cvar/cvar_config_file.h"
#include "workshop.core/cvar/cvar_config_lexer.h"
#include "workshop.core/cvar/cvar_config_parser.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/filesystem/stream.h"

namespace ws {

bool cvar_config_file::parse(const char* path)
{
    std::unique_ptr<stream> stream = virtual_file_system::get().open(path, false);
    if (!stream)
    {
        return false;
    }

    std::string contents = stream->read_all_string();

    cvar_config_lexer lexer;
    if (!lexer.lex(contents.c_str(), path))
    {
        return false;
    }

    cvar_config_parser parser;
    if (!parser.parse(lexer.get_tokens(), path))
    {
        return false;
    }

    m_ast_nodes = std::move(parser.take_ast_nodes());

    return true;
}

bool cvar_config_file::evaluate_defaults()
{
    cvar_config_ast_eval_context ctx;
    ctx.assign_defaults_only = true;
    m_ast_nodes[0]->evaluate(ctx);
    return true;
}

bool cvar_config_file::evaluate()
{
    cvar_config_ast_eval_context ctx;
    ctx.assign_non_defaults_only = true;
    m_ast_nodes[0]->evaluate(ctx);
    return true;
}

}; // namespace ws
