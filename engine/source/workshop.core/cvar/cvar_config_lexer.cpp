// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/cvar/cvar_config_lexer.h"
#include "workshop.core/debug/debug.h"

#include <cctype>

namespace ws {

bool cvar_config_lexer::lex(const char* text, const char* path)
{
    m_path = path;
    m_text = text;
    m_position = 0;
    m_current_line = 1;
    m_current_column = 1;

    while (!end_of_text())
    {
        size_t start_position = m_position;
        char c = next_char();

        // Identifier
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
        {
            if (!read_literal_identifier())
            {
                return false;
            }
        }
        // Number
        else if (c >= '0' && c <= '9')
        {
            if (!read_literal_number())
            {
                return false;
            }
        }
        // String
        else if (c == '\"')
        {
            if (!read_literal_string())
            {
                return false;
            }
        }
        // Whitespace
        else if (iswspace(c))
        {
            continue;
        }
        // Comment
        else if (c == '/' && look_ahead_char() == '/')
        {
            while (!end_of_text() && next_char() != '\n')
            {
                // Read to end of line (or file).
            }
        }
        // Operators
        else if (c == '>')
        {
            if (look_ahead_char() == '=')
            {
                next_char();
                new_token(cvar_config_token_type::operator_greater_equal, start_position, m_position);
            }
            else
            {
                new_token(cvar_config_token_type::operator_greater, start_position, m_position);
            }
        }
        else if (c == '<')
        {
            if (look_ahead_char() == '=')
            {
                next_char();
                new_token(cvar_config_token_type::operator_less_equal, start_position, m_position);
            }
            else
            {
                new_token(cvar_config_token_type::operator_less, start_position, m_position);
            }
        }
        else if (c == '=')
        {
            if (look_ahead_char() == '=')
            {
                next_char();
                new_token(cvar_config_token_type::operator_equal, start_position, m_position);
            }
            else
            {
                new_token(cvar_config_token_type::operator_assign, start_position, m_position);
            }
        }
        else if (c == '!' && look_ahead_char() == '=')
        {
            next_char();
            new_token(cvar_config_token_type::operator_not_equal, start_position, m_position);         
        }
        else if (c == '&' && look_ahead_char() == '&')
        {
            next_char();
            new_token(cvar_config_token_type::operator_and, start_position, m_position);
        }
        else if (c == '|' && look_ahead_char() == '|')
        {
            next_char();
            new_token(cvar_config_token_type::operator_or, start_position, m_position);
        }
        else if (c == '{')
        {
            new_token(cvar_config_token_type::brace_open, start_position, m_position);
        }
        else if (c == '}')
        {
            new_token(cvar_config_token_type::brace_close, start_position, m_position);
        }
        else if (c == '(')
        {
            new_token(cvar_config_token_type::parenthesis_open, start_position, m_position);
        }
        else if (c == ')')
        {
            new_token(cvar_config_token_type::parenthesis_close, start_position, m_position);
        }
        else if (c == ';')
        {
            new_token(cvar_config_token_type::semicolon, start_position, m_position);
        }
        else
        {
            db_error(core, "[%s:%zi] Unexpected character '%c'.", m_path.c_str(), m_current_line,  c);
            return false;
        }
    }

    return true;
}

bool cvar_config_lexer::read_literal_identifier()
{
    size_t start_index = (m_position - 1);
    size_t end_index = start_index + 1;

    while (!end_of_text())
    {
        char c = look_ahead_char();
        if ((c >= 'a' && c <= 'z') || 
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '_')
        {
            next_char();
            end_index++;
        }
        else
        {
            break;
        }
    }

    std::string_view text = std::string_view(m_text.data() + start_index, m_text.data() + start_index + (end_index - start_index));
    cvar_config_token_type type = cvar_config_token_type::literal_identifier;

    for (auto& [ value, keyword_type ] : m_keyword_lookup)
    {
        if (value == text)
        {
            type = keyword_type;
            break;
        }
    }

    new_token(type, start_index, end_index);

    return true;
}

bool cvar_config_lexer::read_literal_number()
{
    size_t start_index = (m_position - 1);
    size_t end_index = start_index + 1;

    bool found_radix = false;

    while (!end_of_text())
    {
        char c = look_ahead_char();
        if ((c >= '0' && c <= '9') || (c == '.' && !found_radix))
        {
            if (c == '.')
            {
                found_radix = true;
            }

            next_char();
            end_index++;
        }
        else
        {
            break;
        }
    }

    new_token(found_radix ? cvar_config_token_type::literal_float : cvar_config_token_type::literal_int, start_index, end_index);

    return true;
}

bool cvar_config_lexer::read_literal_string()
{
    size_t start_index = (m_position - 1);
    size_t end_index = start_index + 1;

    std::string unescaped_string = "";

    while (!end_of_text())
    {
        char c = look_ahead_char();

        // End of string
        if (c == '\"')
        {
            next_char();
            break;
        }
        // Escaped character
        else if (c == '\\')
        {
            next_char();
            end_index++;

            if (!end_of_text())
            {
                unescaped_string += next_char();
            }
            else
            {
                db_error(core, "[%s:%zi] Encountered end of file when reading escaped character.", m_path.c_str(), m_current_line);
                return false;
            }
        }
        // String character
        else
        {
            next_char();
            end_index++;

            unescaped_string += c;
        }
    }

    new_token(cvar_config_token_type::literal_string, start_index, end_index, unescaped_string);

    return true;
}

void cvar_config_lexer::new_token(cvar_config_token_type type, size_t start_index, size_t end_index, const std::string& raw_text)
{
    cvar_config_token& token = m_tokens.emplace_back();
    token.type = type;
    if (raw_text.empty())
    {
        token.text = std::string_view(m_text.data() + start_index, m_text.data() + start_index + (end_index - start_index));
    }
    else
    {
        token.text_storage = std::make_unique<std::string>(raw_text);
        token.text = *token.text_storage;
    }
    token.start_index = start_index;
    token.end_index = end_index;
    token.line = m_current_line;
    token.column = m_current_column;
}

char cvar_config_lexer::next_char()
{
    char c = m_text[m_position];
    if (c == '\n')
    {
        m_current_line++;
        m_current_column = 1;
    }
    else if (c != '\r')
    {
        m_current_column++;
    }
    m_position++;
    return c;
}

char cvar_config_lexer::current_char()
{
    if (m_position == 0)
    {
        return '\0';
    }
    return m_text[m_position - 1];
}

char cvar_config_lexer::look_ahead_char()
{
    if (end_of_text())
    {
        return '\0';
    }
    return m_text[m_position];
}

bool cvar_config_lexer::end_of_text()
{
    return m_position >= m_text.length();
}

std::span<cvar_config_token> cvar_config_lexer::get_tokens()
{
    return m_tokens;
}

}; // namespace ws
