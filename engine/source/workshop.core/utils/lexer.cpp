// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/utils/lexer.h"
#include "workshop.core/utils/string_formatter.h"
#include "workshop.core/debug/log.h"
#include <cstring>

namespace ws {

lexer::lexer()
{

}

void lexer::load(const char* input)
{
	m_state.input = input;
	m_state.cursor = input;
}

bool lexer::eof()
{
	return (m_state.cursor[0] == '\0');
}

bool lexer::peek(token& result)
{
	lexer_state state = store_state();
	bool ret = read(result);
	restore_state(state);
	return ret;
}

lexer_state lexer::store_state()
{
	return m_state;
}

void lexer::restore_state(const lexer_state& state)
{
	m_state = state;
}

void lexer::log_error(const char* location, const char* format, ...)
{
    char error_msg[512];

    va_list va;
    va_start(va, format);
    vsnprintf(error_msg, sizeof(error_msg), format, va);
    va_end(va);

    // Find start and end of current line.
    const char* line_start = location;
    const char* line_end = location;

    while (true)
    {
        if (line_start == m_state.cursor || 
            line_start[0] == '\n' || 
            line_start[0] == '\r')
        {
            break;
        }
        line_start--;
    }
        
    while (true)
    {
        if (line_end[0] == '\0' ||
            line_end[0] == '\n' ||
            line_end[0] == '\r')
        {
            break;
        }
        line_end++;
    }

    size_t line_offset = std::distance(line_start, location);
    size_t line_length = std::distance(line_start, line_end);

    char line[512];
    char cursor_line[512];

    // Not enougth space to print the line information so just log the error. 
    if (line_length >= sizeof(line) - 1)
    {
        db_error(core, "%s", error_msg);
    }

    strncpy(line, line_start, line_length);
    memset(cursor_line, ' ', line_offset);
    cursor_line[line_offset] = '^';
    cursor_line[line_offset + 1] = '\0';

    db_error(core, "%s\n%s\n%s", error_msg, line, cursor_line);
}

bool lexer::read(token& result)
{
	// Consume whitespace before token.
	while (true)
	{
		if (m_state.cursor[0] == ' ' || 
			m_state.cursor[0] == '\t' || 
			m_state.cursor[0] == '\n' || 
			m_state.cursor[0] == '\r')
		{
			m_state.cursor++;
		}
		else
		{
			break;
		}
	}

    if (eof())
    {
        return false;
    }

	char c = m_state.cursor[0];
	const char* start = m_state.cursor;

    m_state.cursor++;

	switch (c)
	{
        case '*': result.type = token_type::op_mul;                 break;
        case '/': result.type = token_type::op_div;                 break;
        case '%': result.type = token_type::op_mod;                 break;
        case '+': result.type = token_type::op_add;                 break;
        case '-': result.type = token_type::op_sub;                 break;
        case ',': result.type = token_type::op_comma;               break;
        case '(': result.type = token_type::op_parenthesis_open;    break;
        case ')': result.type = token_type::op_parenthesis_close;   break;
        case '~': result.type = token_type::op_bitwise_not;         break;
        case '^': result.type = token_type::op_bitwise_xor;         break;
        case '=': 
        {
            if (m_state.cursor[0] == '=') 
            {
                result.type = token_type::op_equal;
                m_state.cursor++;
            } 
            else 
            {
                result.type = token_type::op_assign;
            }
            break;
        }
        case '<': 
        {
            if (m_state.cursor[0] == '=') 
            {
                result.type = token_type::op_le;
                m_state.cursor++;
            } 
            else 
            {
                result.type = token_type::op_less;
            }
            break;
        }
        case '>': 
        {
            if (m_state.cursor[0] == '=') 
            {
                result.type = token_type::op_ge;
                m_state.cursor++;
            }
            else 
            {
                result.type = token_type::op_greater;
            }
            break;
        }
        case '!': 
        {
            if (m_state.cursor[0] == '=') 
            {
                result.type = token_type::op_not_equal;
                m_state.cursor++;
            }
            else 
            {
                result.type = token_type::op_not;
            }
            break;
        }
        case '&': 
        {
            if (m_state.cursor[0] == '&') 
            {
                result.type = token_type::op_logical_and;
                m_state.cursor++;
            }
            else 
            {
                result.type = token_type::op_bitwise_and;
            }
            break;
        }
        case '|': 
        {
            if (m_state.cursor[0] == '|') 
            {
                result.type = token_type::op_logical_or;
                m_state.cursor++;
            }
            else 
            {
                result.type = token_type::op_bitwise_or;
            }
            break;
        }
        case '"': 
        {
            result.type = token_type::literal_string;
            result.buffer[0] = '\0';

            int len = 0;
            while (1) 
            {
                c = m_state.cursor[0];
                m_state.cursor++;

                if (c == '\0') 
                {
                    log_error(start, "Unterminated string");
                    return false;
                } 
                else if (c == '"') 
                {
                    break;
                } 
                else if (c == '\\') 
                {
                    if (len >= sizeof(result.buffer) - 1)
                    {
                        log_error(start, "Token too long to store.");
                        return false;
                    }

                    char escape_c = m_state.cursor[0];
                    m_state.cursor++;

                    switch (escape_c) {
                        case '"':   break;
                        case 'a':   result.buffer[len++] = '\a'; break;
                        case 'b':   result.buffer[len++] = '\b'; break;
                        case 'f':   result.buffer[len++] = '\f'; break;
                        case 'n':   result.buffer[len++] = '\n'; break;
                        case 'r':   result.buffer[len++] = '\r'; break;
                        case 't':   result.buffer[len++] = '\t'; break;
                        case 'v':   result.buffer[len++] = '\v'; break;
                        case '\\':  result.buffer[len++] = '\\'; break;
                        case '\?':  result.buffer[len++] = '?';  break;
                        // TODO: Maybe support hex/octal/unicode values here?
                        default: 
                        {
                            log_error(m_state.cursor, "Found unexpected escape sequence '%c'", escape_c);
                            return false;
                        }
                    }
                } 
                else 
                {
                    if (len >= sizeof(result.buffer) - 1)
                    {
                        log_error(start, "Token too long to store.");
                        return false;
                    }

                    result.buffer[len++] = c;
                }
            }

            result.buffer[len] = '\0';
            break;
        }
        default: 
        {
            // Identifier
            if ((c >= 'a' && c <= 'z') || 
                (c >= 'A' && c <= 'Z') || 
                 c == '_') 
            {
                result.type = token_type::literal_identifier;

                int len = 0;
                do 
                {
                    c = m_state.cursor[0];
                    if ( ! ((c >= 'a' && c <= 'z') || 
                            (c >= 'A' && c <= 'Z') || 
                            (c >= '0' && c <= '9') || 
                             c == '_') ) 
                    {
                        break;
                    }
                    m_state.cursor++;
                } 
                while (m_state.cursor[0]);
            }
            // Number (or + / - operator)
            else if ((c >= '0' && c <= '9') || c == '.') 
            {
                result.type = token_type::literal_int;

                int found_hex = 0;
                int found_exponent = 0;
                do 
                {
                    c = m_state.cursor[0];
                    if (c >= '0' && c <= '9') 
                    {
                        // Just consume these
                    } 
                    else if (found_hex && ((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) 
                    {
                        // Just consume these
                    } 
                    else if (c == '.') 
                    {
                        if (result.type == token_type::literal_float)
                        {
                            log_error(m_state.cursor, "Floating point values cannot contain multiple radix's");
                            return false;
                        }
                        result.type = token_type::literal_float;
                    } 
                    else if (c == 'e') 
                    {
                        if (found_hex) 
                        {
                            log_error(m_state.cursor, "Exponent values cannot have a hex prefix");
                            return false;
                        }
                        if (found_exponent) 
                        {
                            log_error(m_state.cursor, "Number values cannot contain multiple exponent values");
                            return false;
                        }
                        found_exponent = 1;
                        result.type = token_type::literal_float;

                        if (m_state.cursor[1] == '-' || m_state.cursor[1] == '+') 
                        {
                            m_state.cursor++;
                        }
                    } 
                    else if (c == 'f') 
                    {
                        m_state.cursor++; // Skip and ignore the f
                        result.type = token_type::literal_float;
                        break;
                    } 
                    else if (c == 'x' || c == 'X') 
                    {
                        result.type = token_type::literal_int;
                        if (found_exponent) 
                        {
                            log_error(m_state.cursor, "Exponent values cannot have a hex prefix");
                            return false;
                        }
                        if (found_hex) 
                        {
                            log_error(m_state.cursor, "Number values cannot contain multiple hex prefixes");
                            return false;
                        }
                        size_t distance = std::distance(m_state.cursor, start);
                        if (start[0] != '0' || distance != 1) 
                        {
                            log_error(m_state.cursor, "Invalid hex prefix in number");
                            return false;
                        }
                        found_hex = 1;
                    } 
                    else 
                    {
                        break;
                    }
                    m_state.cursor++;
                } 
                while (m_state.cursor[0]);
            }
            // Unknown
            else {
                log_error(m_state.cursor, "Encountered unexpected character '%c'");
                return false;
            }
        }
	}

    // Copy the token to the buffer (except for strings which do this manually for escaping).
    if (result.type != token_type::literal_string)
    {
        size_t length = std::distance(start, m_state.cursor); // Ignore the next character.
        if (length
            >= sizeof(result.buffer) - 1)
        {
            log_error(start, "Token too long to store.");
            return false;
        }

        memcpy(result.buffer, start, length);
        result.buffer[length] = '\0';
    }

    return true;
}

}; // namespace ws
