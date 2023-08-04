// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <array>
#include <vector>
#include <stdarg.h>

namespace ws {

// ================================================================================================
//  This class allows you to format a string on a fixed block of memory on the stack, and 
//  as a fallback allocates heap space if enough space is not available.
// ================================================================================================
template <int stack_space = 1024> 
class string_formatter
{
public:

	// Formats varidic argument lists and stores the result in the internal storage retrievable 
	// with c_str();
	void format_va(const char* format, va_list list);
	void format(const char* format, ...);

	// Gets the resulting character string after formatting.
	const char* c_str() const;

private:
	std::vector<char> m_heap_space;
	std::array<char, stack_space> m_stack_space;
	bool m_using_stack = false;

};

template <int stack_space>
inline void string_formatter<stack_space>::format_va(const char* format, va_list list)
{
    char* buffer_to_use = m_stack_space.data();
    m_using_stack = true;

    int ret = vsnprintf(buffer_to_use, m_stack_space.size(), format, list);
    int space_required = ret + 1;

    // If we don't have enough space on the stack, allocate some space on the heap
    // to store our formatted result.
    if (ret >= m_stack_space.size())
    {
        m_heap_space.resize(space_required);
		m_using_stack = false;

        vsnprintf(m_heap_space.data(), space_required, format, list);
    }
}

template <int stack_space>
inline void string_formatter<stack_space>::format(const char* format, ...)
{
	va_list list;
	va_start(list, format);
	format_va(format, list);
	va_end(list);
}

template <int stack_space>
inline const char* string_formatter<stack_space>::c_str() const
{
	return m_using_stack ? m_stack_space.data() : m_heap_space.data();
}

}; // namespace workshop
