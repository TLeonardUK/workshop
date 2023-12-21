// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/cvar/cvar_config_parser.h"
#include "workshop.core/debug/debug.h"

namespace ws {

static inline const char* cvar_config_token_type_strings[(int)cvar_config_token_type::COUNT] = {

    "invalid",
    
    "string",
    "int",
    "float",
    "bool",
    "identifier",
    
    "if",
    "else",
    "default",
    
    ">=",
    ">",
    "<=",
    "<",
    "==",
    "!=",
    "&&",
    "||",
    "=",
    
    "{",
    "}",
    "(",
    ")",
    ";",
};

bool cvar_config_parser::parse(std::span<cvar_config_token> tokens, const char* path)
{
    m_tokens = tokens;
    m_path = path;
    m_position = 0;

    cvar_config_ast_node_block& root_node = make_node<cvar_config_ast_node_block>();

    try
    {
        while (!end_of_tokens())
        {
            cvar_config_token& token = next_token();

            // Top level default block.
            if (token.type == cvar_config_token_type::keyword_default)
            {
                root_node.children.push_back(parse_default());
            }
            // Top level if block.
            else if (token.type == cvar_config_token_type::keyword_if)
            {
                root_node.children.push_back(parse_if());
            }
            // Top level assignment statement.
            else if (token.type == cvar_config_token_type::literal_identifier)
            {
                root_node.children.push_back(parse_assignment());
            }
            else
            {
                unexpected_token(token);
            }
        }
    }
    catch (std::exception)
    {
        // parse error :(
        return false;
    }

    return true;
}

cvar_config_ast_node* cvar_config_parser::parse_default()
{
    cvar_config_ast_node_default& node = make_node<cvar_config_ast_node_default>();
    node.block_node = parse_block();

    return &node;
}

cvar_config_ast_node* cvar_config_parser::parse_if()
{
    cvar_config_ast_node_if& node = make_node<cvar_config_ast_node_if>();

    expect_token(cvar_config_token_type::parenthesis_open);
    node.expression_node = parse_expression();
    expect_token(cvar_config_token_type::parenthesis_close);
    node.block_node = parse_block();

    if (look_ahead_token().type == cvar_config_token_type::keyword_else)
    {
        next_token();

        if (look_ahead_token().type == cvar_config_token_type::keyword_if)
        {
            next_token();
            node.else_node = parse_if();
        }
        else
        {
            node.else_node = parse_block();
        }
    }

    return &node;
}

cvar_config_ast_node_block* cvar_config_parser::parse_block()
{
    cvar_config_ast_node_block& node = make_node<cvar_config_ast_node_block>();

    expect_token(cvar_config_token_type::brace_open);

    while (!end_of_tokens() && look_ahead_token().type != cvar_config_token_type::brace_close)
    {
        node.children.push_back(parse_statement());
    }

    expect_token(cvar_config_token_type::brace_close);

    return &node;
}

cvar_config_ast_node* cvar_config_parser::parse_assignment()
{
    cvar_config_ast_node_assignment& node = make_node<cvar_config_ast_node_assignment>();
    node.lvalue_identifier = current_token().text;

    expect_token(cvar_config_token_type::operator_assign);

    // Read the value being assigned.
    cvar_config_token& token = next_token();
    if (token.type == cvar_config_token_type::literal_string ||
        token.type == cvar_config_token_type::literal_int ||
        token.type == cvar_config_token_type::literal_bool ||
        token.type == cvar_config_token_type::literal_float)
    {
        // OK!
    }
    else
    {
        unexpected_token(token);
    }

    node.rvalue_string = token.text;
    node.rvalue_type = token.type;

    expect_token(cvar_config_token_type::semicolon);

    return &node;
}

cvar_config_ast_node* cvar_config_parser::parse_statement()
{
    cvar_config_token& token = next_token();

    // If block.
    if (token.type == cvar_config_token_type::keyword_if)
    {
        return parse_if();
    }
    // Assignment statement.
    else if (token.type == cvar_config_token_type::literal_identifier)
    {
        return parse_assignment();
    }
    else
    {
        unexpected_token(token);
    }

    return nullptr;
}

cvar_config_ast_node* cvar_config_parser::parse_expression()
{
    cvar_config_ast_node* lvalue = parse_sub_expression();

    while (!end_of_tokens())
    {
        cvar_config_token& token = look_ahead_token();
        if (token.type != cvar_config_token_type::operator_and &&
            token.type != cvar_config_token_type::operator_or)
        {
            break;
        }
        
        next_token();

        cvar_config_ast_node* rvalue = parse_sub_expression();

        cvar_config_ast_node_operator& node = make_node<cvar_config_ast_node_operator>();
        node.type = token.type;
        node.lvalue_node = lvalue;
        node.rvalue_node = rvalue;

        lvalue = &node; 
    }

    return lvalue;
}

cvar_config_ast_node* cvar_config_parser::parse_sub_expression()
{
    cvar_config_ast_node* lvalue = parse_leaf_expression();

    while (!end_of_tokens())
    {
        cvar_config_token& token = look_ahead_token();
        if (token.type != cvar_config_token_type::operator_less &&
            token.type != cvar_config_token_type::operator_less_equal &&
            token.type != cvar_config_token_type::operator_greater &&
            token.type != cvar_config_token_type::operator_greater_equal &&
            token.type != cvar_config_token_type::operator_equal &&
            token.type != cvar_config_token_type::operator_not_equal)
        {
            break;
        }

        next_token();

        cvar_config_ast_node* rvalue = parse_leaf_expression();

        cvar_config_ast_node_operator& node = make_node<cvar_config_ast_node_operator>();
        node.type = token.type;
        node.lvalue_node = lvalue;
        node.rvalue_node = rvalue;

        lvalue = &node;
    }

    return lvalue;
}

cvar_config_ast_node* cvar_config_parser::parse_leaf_expression()
{
    cvar_config_token& token = next_token();
    if (token.type == cvar_config_token_type::literal_string ||
        token.type == cvar_config_token_type::literal_int ||
        token.type == cvar_config_token_type::literal_float ||
        token.type == cvar_config_token_type::literal_bool ||
        token.type == cvar_config_token_type::literal_identifier)
    {
        cvar_config_ast_node_literal& node = make_node<cvar_config_ast_node_literal>();
        node.rvalue_string = token.text;
        node.rvalue_type = token.type;

        return &node;
    }
    else
    {
        unexpected_token(token);

        return nullptr;
    }
}

bool cvar_config_parser::end_of_tokens()
{
    return m_position >= m_tokens.size();
}

void cvar_config_parser::expect_token(cvar_config_token_type type)
{
    cvar_config_token& token = next_token();
    
    if (token.type != type)
    {
        db_error(core, "[%s:%zi] Unexpected token '%s', expected '%s'.", 
            m_path.c_str(), 
            token.line, 
            std::string(token.text).c_str(),
            cvar_config_token_type_strings[(int)type]
        );

        throw std::exception();
    }
 }

void cvar_config_parser::unexpected_token(cvar_config_token& token)
{
    db_error(core, "[%s:%zi] Unexpected token '%s'.", m_path.c_str(), token.line, std::string(token.text).c_str());

    throw std::exception();
}

cvar_config_token& cvar_config_parser::next_token()
{
    if (end_of_tokens())
    {
        return m_invalid_token;
    }

    cvar_config_token& c = m_tokens[m_position];
    m_position++;
    return c;
}

cvar_config_token& cvar_config_parser::current_token()
{
    if (m_position == 0)
    {
        return m_invalid_token;
    }
    return m_tokens[m_position - 1];
}

cvar_config_token& cvar_config_parser::look_ahead_token()
{
    if (end_of_tokens())
    {
        return m_invalid_token;
    }
    return m_tokens[m_position];
}

std::vector<std::unique_ptr<cvar_config_ast_node>>&& cvar_config_parser::take_ast_nodes()
{
    return std::move(m_ast_nodes);
}

}; // namespace ws
