// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/cvar/cvar_config_lexer.h"
#include "workshop.core/cvar/cvar_config_ast.h"

namespace ws {

// Parses a string of tokens into an abstract form that can be evalulated by 
// config system.
class cvar_config_parser
{
public:
    // Parses a stream of tokens. Returns true on success.
    bool parse(std::span<cvar_config_token> tokens, const char* path);

    // Gets the AST tree that was generated while parsing. The first
    // element is the root node.
    std::vector<std::unique_ptr<cvar_config_ast_node>>&& take_ast_nodes();

private:
    bool end_of_tokens();
    cvar_config_token& next_token();
    cvar_config_token& current_token();
    cvar_config_token& look_ahead_token();

    void expect_token(cvar_config_token_type type);

    cvar_config_ast_node* parse_default();
    cvar_config_ast_node* parse_if();
    cvar_config_ast_node* parse_assignment();
    cvar_config_ast_node* parse_statement();
    cvar_config_ast_node_block* parse_block();

    cvar_config_ast_node* parse_expression();
    cvar_config_ast_node* parse_sub_expression();
    cvar_config_ast_node* parse_leaf_expression();

    void unexpected_token(cvar_config_token& token);

    template <typename node_type>
    node_type& make_node()
    {
        std::unique_ptr<node_type> ptr = std::make_unique<node_type>();
        node_type& ref = *ptr;

        m_ast_nodes.push_back(std::move(ptr));

        return ref;
    }

private:
    std::string m_path;
    
    cvar_config_token m_invalid_token = { };
    std::span<cvar_config_token> m_tokens;

    size_t m_position;

    std::vector<std::unique_ptr<cvar_config_ast_node>> m_ast_nodes;

};

}; // namespace ws
