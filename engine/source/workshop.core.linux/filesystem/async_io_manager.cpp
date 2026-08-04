// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/async_io_manager.h"
#include "workshop.core/math/math.h"
#include "workshop.core/utils/time.h"
#include "workshop.core.linux/utils/windows_headers.h"
#include "workshop.core.linux/filesystem/async_io_manager.h"
#include "workshop.core/memory/memory_tracker.h"
#include "workshop.core/perf/profile.h"

#include <filesystem>
#include <span>
#include <mutex>
#include <cstddef>

namespace ws {

// linux-todo

std::unique_ptr<async_io_manager> async_io_manager::create()
{
    return std::make_unique<linux_async_io_manager>();
}

linux_async_io_request::linux_async_io_request(linux_async_io_manager* manager, const char* path, size_t offset, size_t size, async_io_request_options options)
    : m_manager(manager)
    , m_path(path)
    , m_offset(offset)
    , m_size(size)
    , m_options(options)
{
}

linux_async_io_request::~linux_async_io_request()
{
}

bool linux_async_io_request::is_complete()
{
    return false;
}

bool linux_async_io_request::has_failed()
{
    return false;
}

void linux_async_io_request::set_state(state new_state)
{
}

std::span<uint8_t> linux_async_io_request::data()
{
    return {};
}

linux_async_io_manager::linux_async_io_manager()
{
}

linux_async_io_manager::~linux_async_io_manager()
{
}

void linux_async_io_manager::worker_thread()
{
    db_set_thread_name("async io manager");
}

float linux_async_io_manager::get_current_bandwidth()
{
    return 0.0f;
}

async_io_request::ptr linux_async_io_manager::request(const char* path, size_t offset, size_t size, async_io_request_options options)
{
    db_assert(false);
    return nullptr;
}

}; // namespace workshop
