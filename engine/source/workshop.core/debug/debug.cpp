// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/debug/debug.h"
#include "workshop.core/containers/string.h"

#include <stdarg.h>

namespace ws {

void db_assert_failed(const char* expression, const char* file, size_t line, const char* msg_format, ...)
{
    db_error(core, "");
    db_error(core, "--- ASSERT FAILED ---");
    db_error(core, "Expression: %s", expression);
    db_error(core, "Location: %s:%zi", file, line);

    if (msg_format != nullptr)
    {
        constexpr size_t k_stack_space = 1024;

        char buffer[k_stack_space];
        char* buffer_to_use = buffer;

        va_list list;
        va_start(list, msg_format);

        int ret = vsnprintf(buffer_to_use, k_stack_space, msg_format, list);
        int space_required = ret + 1;
        if (ret >= k_stack_space)
        {
            buffer_to_use = new char[space_required];
            vsnprintf(buffer_to_use, space_required, msg_format, list);
        }

        db_error(core, "Message: %s", buffer_to_use);

        if (buffer_to_use != buffer)
        {
            delete[] buffer_to_use;
        }

        va_end(list);
    }

    // TODO: Option to ignore/abort

    db_error(core, "Callstack:");
    std::unique_ptr<db_callstack> callstack = db_capture_callstack(1);
    for (size_t i = 0; i < callstack->frames.size(); i++)
    {
        db_callstack::frame& frame = callstack->frames[i];
        if (frame.function.empty())
        {
            db_error(core, "[%zi] 0x%p", i, frame.address);
        }
        else
        {
            db_error(core, "[%zi] 0x%p %s!%s (%s:%zi)", i, frame.address, frame.module.c_str(), frame.function.c_str(), frame.filename.c_str(), frame.line);
        }
    }

    db_break();
    db_terminate();
}

}; // namespace workshop
