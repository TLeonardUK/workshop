// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/cvar/cvar_config_lexer.h"
#include <variant>

namespace ws {

// Context used when evaluating state of ast nodes.
struct cvar_config_ast_eval_context
{
    bool in_default_block = false;

    bool assign_defaults_only = false;
    bool assign_non_defaults_only = false;
};

// Base class for all ast nodes.
class cvar_config_ast_node
{
public:
    virtual ~cvar_config_ast_node() {};

    using evaluate_result = std::variant<bool, int, float, std::string>;

    virtual evaluate_result evaluate(cvar_config_ast_eval_context& ctx) = 0;

    static bool coerce_result_to_bool(const evaluate_result& ret);
    static int coerce_result_to_int(const evaluate_result& ret);
    static float coerce_result_to_float(const evaluate_result& ret);
    static std::string coerce_result_to_string(const evaluate_result& ret);
};

// Node for a block of statements.
class cvar_config_ast_node_block : public cvar_config_ast_node
{
public:
    std::vector<cvar_config_ast_node*> children;

public:
    virtual evaluate_result evaluate(cvar_config_ast_eval_context& ctx) override;
};

// Node for default block.
class cvar_config_ast_node_default : public cvar_config_ast_node
{
public:
    cvar_config_ast_node_block* block_node = nullptr;

public:
    virtual evaluate_result evaluate(cvar_config_ast_eval_context& ctx) override;
};

// Node for if block
class cvar_config_ast_node_if : public cvar_config_ast_node
{
public:
    cvar_config_ast_node* expression_node = nullptr;
    cvar_config_ast_node* block_node = nullptr;
    cvar_config_ast_node* else_node = nullptr;

public:
    virtual evaluate_result evaluate(cvar_config_ast_eval_context& ctx) override;
};

// Node for an assignment
class cvar_config_ast_node_assignment : public cvar_config_ast_node
{
public:
    std::string lvalue_identifier = "";

    std::string rvalue_string = "";
    cvar_config_token_type rvalue_type;

public:
    virtual evaluate_result evaluate(cvar_config_ast_eval_context& ctx) override;
};

// Node for an operator > == & / etc expression
class cvar_config_ast_node_operator : public cvar_config_ast_node
{
public:
    cvar_config_token_type type;
    cvar_config_ast_node* lvalue_node = nullptr;
    cvar_config_ast_node* rvalue_node = nullptr;

public:
    virtual evaluate_result evaluate(cvar_config_ast_eval_context& ctx) override;
};

// Node for a literal value
class cvar_config_ast_node_literal : public cvar_config_ast_node
{
public:
    std::string rvalue_string = "";
    cvar_config_token_type rvalue_type;

public:
    virtual evaluate_result evaluate(cvar_config_ast_eval_context& ctx) override;
};

}; // namespace ws
