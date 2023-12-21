// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/cvar/cvar_config_ast.h"
#include "workshop.core/cvar/cvar_manager.h"
#include "workshop.core/cvar/cvar.h"
#include "workshop.core/debug/debug.h"

namespace ws {

bool cvar_config_ast_node::coerce_result_to_bool(const evaluate_result& ret)
{
    if (ret.index() == 0)
    {
        return std::get<bool>(ret);
    }
    else if (ret.index() == 1)
    {
        return std::get<int>(ret) != 0;
    }
    else if (ret.index() == 2)
    {
        return std::get<float>(ret) != 0.0f;
    }
    else if (ret.index() == 3)
    {
        return !std::get<std::string>(ret).empty();
    }
    return false;
}

int cvar_config_ast_node::coerce_result_to_int(const evaluate_result& ret)
{
    if (ret.index() == 0)
    {
        return std::get<bool>(ret) ? 1 : 0;
    }
    else if (ret.index() == 1)
    {
        return std::get<int>(ret);
    }
    else if (ret.index() == 2)
    {
        return (int)std::get<float>(ret);
    }
    else if (ret.index() == 3)
    {
        return atoi(std::get<std::string>(ret).c_str());
    }
    return 0;
}

float cvar_config_ast_node::coerce_result_to_float(const evaluate_result& ret)
{
    if (ret.index() == 0)
    {
        return std::get<bool>(ret) ? 1.0f : 0.0f;
    }
    else if (ret.index() == 1)
    {
        return (float)std::get<int>(ret);
    }
    else if (ret.index() == 2)
    {
        return std::get<float>(ret);
    }
    else if (ret.index() == 3)
    {
        return (float)atof(std::get<std::string>(ret).c_str());
    }
    return 0.0f;
}

std::string cvar_config_ast_node::coerce_result_to_string(const evaluate_result& ret)
{
    if (ret.index() == 0)
    {
        return std::get<bool>(ret) ? "1" : "0";
    }
    else if (ret.index() == 1)
    {
        return std::to_string(std::get<int>(ret));
    }
    else if (ret.index() == 2)
    {
        return std::to_string(std::get<float>(ret));
    }
    else if (ret.index() == 3)
    {
        return std::get<std::string>(ret);
    }
    return "";
}

cvar_config_ast_node::evaluate_result cvar_config_ast_node_block::evaluate(cvar_config_ast_eval_context& ctx)
{
    for (auto& child : children)
    {
        child->evaluate(ctx);
    }

    return true;
}

cvar_config_ast_node::evaluate_result cvar_config_ast_node_default::evaluate(cvar_config_ast_eval_context& ctx)
{
    ctx.in_default_block = true;
    block_node->evaluate(ctx);
    ctx.in_default_block = false;

    return false;
}

cvar_config_ast_node::evaluate_result cvar_config_ast_node_if::evaluate(cvar_config_ast_eval_context& ctx)
{
    evaluate_result ret = expression_node->evaluate(ctx);
    
    if (coerce_result_to_bool(ret))
    {
        block_node->evaluate(ctx);
    }
    else if (else_node)
    {
        else_node->evaluate(ctx);
    }

    return false;
}

cvar_config_ast_node::evaluate_result cvar_config_ast_node_assignment::evaluate(cvar_config_ast_eval_context& ctx)
{
    cvar_base* cvar_instance = cvar_manager::get().find_cvar(lvalue_identifier.c_str());
    if (cvar_instance == nullptr)
    {
        db_error(core, "Failed to find cvar '%s' that is specified in cvar config file.", lvalue_identifier.c_str());
        return false;
    }

    if (rvalue_type == cvar_config_token_type::literal_int && cvar_instance->get_value_type() != typeid(int))
    {
        db_error(core, "Attempted to set '%s' to type other than int.", lvalue_identifier.c_str());
        return false;
    }
    else if (rvalue_type == cvar_config_token_type::literal_float && cvar_instance->get_value_type() != typeid(float))
    {
        db_error(core, "Attempted to set '%s' to type other than float.", lvalue_identifier.c_str());
        return false;
    }
    else if (rvalue_type == cvar_config_token_type::literal_bool && cvar_instance->get_value_type() != typeid(bool))
    {
        db_error(core, "Attempted to set '%s' to type other than bool.", lvalue_identifier.c_str());
        return false;
    }
    else if (rvalue_type == cvar_config_token_type::literal_string && cvar_instance->get_value_type() != typeid(std::string))
    {
        db_error(core, "Attempted to set '%s' to type other than string.", lvalue_identifier.c_str());
        return false;
    }

    if (ctx.assign_defaults_only && !ctx.in_default_block)
    {
        return false;
    }
    if (ctx.assign_non_defaults_only && ctx.in_default_block)
    {
        return false;
    }

    cvar_instance->coerce_from_string(rvalue_string.c_str(), ctx.in_default_block ? cvar_source::set_by_config_default : cvar_source::set_by_config);
    return true;
}

cvar_config_ast_node::evaluate_result cvar_config_ast_node_operator::evaluate(cvar_config_ast_eval_context& ctx)
{
    evaluate_result lvalue_result = lvalue_node->evaluate(ctx);
    evaluate_result rvalue_result = rvalue_node->evaluate(ctx);

    switch (lvalue_result.index())
    {
    // bool
    case 0:
        {
            switch (type)
            {
                case cvar_config_token_type::operator_greater_equal: return (int)std::get<bool>(lvalue_result) >= (int)coerce_result_to_bool(rvalue_result);
                case cvar_config_token_type::operator_greater:       return (int)std::get<bool>(lvalue_result) >  (int)coerce_result_to_bool(rvalue_result);
                case cvar_config_token_type::operator_less_equal:    return (int)std::get<bool>(lvalue_result) <= (int)coerce_result_to_bool(rvalue_result);
                case cvar_config_token_type::operator_less:          return (int)std::get<bool>(lvalue_result) <  (int)coerce_result_to_bool(rvalue_result);
                case cvar_config_token_type::operator_equal:         return (int)std::get<bool>(lvalue_result) == (int)coerce_result_to_bool(rvalue_result);
                case cvar_config_token_type::operator_not_equal:     return (int)std::get<bool>(lvalue_result) != (int)coerce_result_to_bool(rvalue_result);
                case cvar_config_token_type::operator_and:           return std::get<bool>(lvalue_result) && coerce_result_to_bool(rvalue_result);
                case cvar_config_token_type::operator_or:            return std::get<bool>(lvalue_result) || coerce_result_to_bool(rvalue_result);
            }
            break;
        }
    // int
    case 1:
        {
            switch (type)
            {
                case cvar_config_token_type::operator_greater_equal: return std::get<int>(lvalue_result) >= coerce_result_to_int(rvalue_result);
                case cvar_config_token_type::operator_greater:       return std::get<int>(lvalue_result) >  coerce_result_to_int(rvalue_result);
                case cvar_config_token_type::operator_less_equal:    return std::get<int>(lvalue_result) <= coerce_result_to_int(rvalue_result);
                case cvar_config_token_type::operator_less:          return std::get<int>(lvalue_result) <  coerce_result_to_int(rvalue_result);
                case cvar_config_token_type::operator_equal:         return std::get<int>(lvalue_result) == coerce_result_to_int(rvalue_result);
                case cvar_config_token_type::operator_not_equal:     return std::get<int>(lvalue_result) != coerce_result_to_int(rvalue_result);
                case cvar_config_token_type::operator_and:           return coerce_result_to_bool(lvalue_result) && coerce_result_to_bool(rvalue_result);
                case cvar_config_token_type::operator_or:            return coerce_result_to_bool(lvalue_result) || coerce_result_to_bool(rvalue_result);
            }
            break;
        }
    // float
    case 2:
        {
            switch (type)
            {
                case cvar_config_token_type::operator_greater_equal: return std::get<float>(lvalue_result) >= coerce_result_to_float(rvalue_result);
                case cvar_config_token_type::operator_greater:       return std::get<float>(lvalue_result) >  coerce_result_to_float(rvalue_result);
                case cvar_config_token_type::operator_less_equal:    return std::get<float>(lvalue_result) <= coerce_result_to_float(rvalue_result);
                case cvar_config_token_type::operator_less:          return std::get<float>(lvalue_result) <  coerce_result_to_float(rvalue_result);
                case cvar_config_token_type::operator_equal:         return std::get<float>(lvalue_result) == coerce_result_to_float(rvalue_result);
                case cvar_config_token_type::operator_not_equal:     return std::get<float>(lvalue_result) != coerce_result_to_float(rvalue_result);
                case cvar_config_token_type::operator_and:           return coerce_result_to_bool(lvalue_result) && coerce_result_to_bool(rvalue_result);
                case cvar_config_token_type::operator_or:            return coerce_result_to_bool(lvalue_result) || coerce_result_to_bool(rvalue_result);
            }
            break;
        }
    // string
    case 3:
        {
            switch (type)
            {
                case cvar_config_token_type::operator_greater_equal: return strcmp(std::get<std::string>(lvalue_result).c_str(), coerce_result_to_string(rvalue_result).c_str()) >= 0;
                case cvar_config_token_type::operator_greater:       return strcmp(std::get<std::string>(lvalue_result).c_str(), coerce_result_to_string(rvalue_result).c_str()) >  0;
                case cvar_config_token_type::operator_less_equal:    return strcmp(std::get<std::string>(lvalue_result).c_str(), coerce_result_to_string(rvalue_result).c_str()) <= 0;
                case cvar_config_token_type::operator_less:          return strcmp(std::get<std::string>(lvalue_result).c_str(), coerce_result_to_string(rvalue_result).c_str()) <  0;
                case cvar_config_token_type::operator_equal:         return strcmp(std::get<std::string>(lvalue_result).c_str(), coerce_result_to_string(rvalue_result).c_str()) == 0;
                case cvar_config_token_type::operator_not_equal:     return strcmp(std::get<std::string>(lvalue_result).c_str(), coerce_result_to_string(rvalue_result).c_str()) != 0;
                case cvar_config_token_type::operator_and:           return coerce_result_to_bool(lvalue_result) && coerce_result_to_bool(rvalue_result);
                case cvar_config_token_type::operator_or:            return coerce_result_to_bool(lvalue_result) || coerce_result_to_bool(rvalue_result);
            }
            break;
        }
    }

    return false;
}

cvar_config_ast_node::evaluate_result cvar_config_ast_node_literal::evaluate(cvar_config_ast_eval_context& ctx)
{
    if (rvalue_type == cvar_config_token_type::literal_int)
    {
        return atoi(rvalue_string.c_str());
    }
    else if (rvalue_type == cvar_config_token_type::literal_float)
    {
        return (float)atof(rvalue_string.c_str());
    }
    else if (rvalue_type == cvar_config_token_type::literal_bool)
    {
        return (_stricmp(rvalue_string.c_str(), "true") == 0 || _stricmp(rvalue_string.c_str(), "1") == 0);
    }
    else if (rvalue_type == cvar_config_token_type::literal_string)
    {
        return rvalue_string;
    }
    else if (rvalue_type == cvar_config_token_type::literal_identifier)
    {
        cvar_base* cvar_instance = cvar_manager::get().find_cvar(rvalue_string.c_str());
        if (cvar_instance == nullptr)
        {
            db_error(core, "Failed to find cvar '%s' that is specified in cvar config file.", rvalue_string.c_str());
            return false;
        }

        if (cvar_instance->get_value_type() == typeid(int))
        {
            return cvar_instance->get_int();
        }
        else if (cvar_instance->get_value_type() == typeid(float))
        {
            return cvar_instance->get_float();
        }
        else if (cvar_instance->get_value_type() == typeid(bool))
        {
            return cvar_instance->get_bool();
        }
        else if (cvar_instance->get_value_type() == typeid(std::string))
        {
            return cvar_instance->get_string();
        }
    }

    return false;
}

}; // namespace ws
