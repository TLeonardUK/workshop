// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <cstddef>

namespace ws {
	
enum class token_type
{
    op_comma,
    op_mul,
    op_div,
    op_sub,
    op_add,
    op_mod,
    op_less,
    op_greater,
    op_le,
    op_ge,
    op_equal,
    op_not_equal,
    op_assign,
    op_not,
    op_logical_and,
    op_logical_or,
    op_bitwise_and,
    op_bitwise_or,
    op_bitwise_not,
    op_bitwise_xor,
    op_parenthesis_open,
    op_parenthesis_close,
    op_semicolon,

	literal_identifier,
	literal_string,
	literal_int,
	literal_float,
	litearl_bool
};

struct token
{
    token_type type;
    char buffer[512];
};

// Stores a point in the input stream that can be used to reset 
// the lexer back to this point.
struct lexer_state
{ 
    const char* input = nullptr;
    const char* cursor = nullptr;
};

// ================================================================================================
//  General purpose lexer 
// ================================================================================================
class lexer
{
public:
    lexer();

    // Loads an input stream from a null terminated string.
	// The input string must remain valid for the lifetime of the lexer.
    void load(const char* input);

	// Returns true if we have reached the end of the input stream.
    bool eof();

    // Reads the next token in the input stream. 
    // Returns true if a token was read, false if we got to the end of the input.
    bool read(token& result);

    // Reads the next token in the input stream. 
    // Returns true if a token was read and is the expected type, false otherwise.
    bool expect(token& result, token_type type);

    // Peeks the next token in the input stream. 
    // Returns true if a token was read, false if we got to the end of the input.
    bool peek(token& result);

    // Saves the current position in the input stream.
    lexer_state store_state();

    // Resets the lexer back to a previously saved position.
    void restore_state(const lexer_state& state);

    // Gets the current location the lexer is at.
    const char* get_cursor();

private:
    void log_error(const char* location, const char* format, ...);

private:
    lexer_state m_state;

};

}; // namespace ws
