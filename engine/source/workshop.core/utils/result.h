// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/debug/log.h"

namespace ws {

// ================================================================================================
//  Represents the most common ways an operation can fail. Result from result to indicate
//  a failure. Extended or custom enums can also be returned from a result by changing
//  the failure reason template argument.
// ================================================================================================
enum class standard_errors
{
    // Generic
    failed = 1,
    out_of_memory,
    permission_denied,
    incorrect_format,
    incorrect_length,
    invalid_parameter,
    invalid_state,
    not_found,
    timeout,
    cancelled,
    not_enough_data,
    already_in_progress,
    malformed_response,
    no_implementation,

    // IO
    failed_to_create_directory,
    failed_to_remove_directory,
    open_file_failed,
    path_not_relative
};

// ================================================================================================
//  Represents a result of an arbitrary type from an operation.
//  Cast to bool to determine success of operation.
//    If operation succeeds then get_result will return a valid result of type result_type.
//    If operation fails then get_error will provide a failure reason from error_type enum.
// ================================================================================================
template <typename result_type = void, typename error_type = standard_errors>
struct result
{
public:

    result()
    {
    }

    result(const result_type& in_result)
        : m_was_success(true)
        , m_result(in_result)
    {
    }

    result(error_type in_error)
        : m_was_success(false)
        , m_error(in_error)
    {
    }

    operator bool()
    {
        return m_was_success;
    }

    const result_type& get()
    {
        db_assert(m_was_success);
        return m_result;
    }

    const result_type& get_result()
    {
        db_assert(m_was_success);
        return m_result;
    }

    error_type get_error()
    {
        db_assert(!m_was_success);
        return m_error;
    }

private:

    bool m_was_success = false;
    error_type m_error;
    result_type m_result;

};

// ================================================================================================
// Partialize specialization of result for void results.
// ================================================================================================
template <typename error_type>
struct result<void, error_type>
{
public:

    result() = delete;

    result(bool was_success)
        : m_was_success(was_success)
    {
    }

    result(error_type in_error)
        : m_was_success(false)
        , m_error(in_error)
    {
    }

    operator bool()
    {
        return m_was_success;
    }

    error_type get_error()
    {
        db_assert(!m_was_success);
        return m_error;
    }

private:

    bool m_was_success = false;
    error_type m_error;

};

}; // namespace workshop
