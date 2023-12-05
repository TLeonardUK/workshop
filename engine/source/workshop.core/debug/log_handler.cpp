// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/debug/log_handler.h"
#include "workshop.core/utils/string_formatter.h"
#include "workshop.core/platform/platform.h"

#include <array>
#include <algorithm>
#include <ctime>
#include <stdarg.h>
#include <queue>

// If set the memory usage will be placed in all log messages.
#define SHOW_MEMORY_IN_LOGS 1

// If se async writing will happen on a background thread to avoid spikes when writing to output.
#define USE_ASYNC_CONSOLE_LOGGING 1

namespace {

// Writes log asyncronously to the output. 
// This is used to avoid large spikes as log messages are queued to the terminal/debugger/file/etc.
class async_log_queue
{
public:
    async_log_queue()
    {
        m_thread = std::make_unique<std::thread>([this]() {
            ws::db_set_thread_name("async log writer");
            worker_thread();
        });
    }

    ~async_log_queue()
    {
        m_running = false;
        m_condvar.notify_all();
        m_thread->join();
    }

    void write_log(ws::log_level level, ws::log_source source, const char* log)
    {
        std::unique_lock lock(m_mutex);

        entry new_entry;
        new_entry.level = level;
        new_entry.source = source;
        new_entry.message = log;

        m_queue.push(new_entry);

        m_condvar.notify_all();
    }

    void worker_thread()
    {
        while (m_running)
        {
            std::queue<entry> log_queue;

            // Wait till we get some messages to log.
            while (m_running)
            {
                std::unique_lock lock(m_mutex);
                if (!m_queue.empty())
                {
                    log_queue = m_queue;
                    m_queue = {};
                    break;
                }
                m_condvar.wait(lock);
            }

            // Write out all log messages.
            while (!log_queue.empty())
            {
                entry& value = log_queue.front();

                ws::log_handler::static_write_formatted_to_handlers(value.level, value.source, value.message.c_str());

                log_queue.pop();
            }
        }
    }

private:
    struct entry
    {
        ws::log_level level;
        ws::log_source source;
        std::string message;
    };

    bool m_running = true;

    std::unique_ptr<std::thread> m_thread;
    std::mutex m_mutex;
    std::condition_variable m_condvar;
    std::queue<entry> m_queue;

};

static async_log_queue m_log_queue;

};

namespace ws {

void log_handler::set_max_log_level(log_level level)
{
    m_max_log_level = level;
}

log_level log_handler::get_max_log_level()
{
    return m_max_log_level;
}

log_handler::log_handler()
{
    std::scoped_lock lock(m_handlers_mutex);

    m_handlers.push_back(this);
}

log_handler::~log_handler()
{
    std::scoped_lock lock(m_handlers_mutex);

    if (auto iter = std::find(m_handlers.begin(), m_handlers.end(), this); iter != m_handlers.end())
    {
        m_handlers.erase(iter);
    }
}

void log_handler::static_write_formatted(log_level level, log_source source, const char* log)
{
#if USE_ASYNC_CONSOLE_LOGGING
    m_log_queue.write_log(level, source, log);
#else
    static_write_formatted_to_handlers(level, source, log);
#endif
}

void log_handler::static_write_formatted_to_handlers(log_level level, log_source source, const char* log)
{
    time_t current_time = time(0);
    struct tm current_time_tstruct;
    localtime_s(&current_time_tstruct, &current_time);

    char time_buffer[32];
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %X", &current_time_tstruct);

    string_formatter formatter;

#if SHOW_MEMORY_IN_LOGS
    size_t memory_bytes = get_memory_usage();
    formatter.format("%s \xB3 %-5zi MB \xB3 %-7s \xB3 %-18s \xB3 %s\n", time_buffer, memory_bytes / (1024 * 1024), log_level_strings[(int)level], log_source_strings[(int)source], log);
#else
    formatter.format("%s \xB3 %-7s \xB3 %-18s \xB3 %s\n", time_buffer, log_level_strings[(int)level], log_source_strings[(int)source], log);
#endif

    {
        std::scoped_lock lock(m_handlers_mutex);

        for (log_handler* handler : m_handlers)
        {
            handler->write_raw(level, source, time_buffer, log);
            handler->write(level, formatter.c_str());
        }
    }
}

void log_handler::static_write(log_level level, log_source source, const char* format, ...)
{
    if (level > m_max_log_level)
    {
        return;
    }

    char buffer[k_stack_space];
    char* buffer_to_use = buffer;

    va_list list;
    va_start(list, format);

    int ret = vsnprintf(buffer_to_use, k_stack_space, format, list);
    int space_required = ret + 1;
    if (ret >= k_stack_space)
    {
        buffer_to_use = new char[space_required];
        vsnprintf(buffer_to_use, space_required, format, list);
    }

    static_write_formatted(level, source, buffer_to_use);

    if (buffer_to_use != buffer)
    {
        delete[] buffer_to_use;
    }

    va_end(list);
}

}; // namespace workshop
