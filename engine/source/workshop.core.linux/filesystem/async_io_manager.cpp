// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/async_io_manager.h"
#include "workshop.core/math/math.h"
#include "workshop.core/utils/time.h"
#include "workshop.core.linux/filesystem/async_io_manager.h"
#include "workshop.core/memory/memory_tracker.h"
#include "workshop.core/perf/profile.h"

#include <filesystem>
#include <span>
#include <mutex>
#include <cstddef>
#include <cstring>
#include <chrono>

#include <fcntl.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <errno.h>

namespace ws {

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
    if (m_buffer)
    {
        m_manager->free_buffer(m_buffer);
        m_buffer = nullptr;
    }
}

bool linux_async_io_request::is_complete()
{
    return (m_state == state::completed || m_state == state::failed);
}

bool linux_async_io_request::has_failed()
{
    return (m_state == state::failed);
}

void linux_async_io_request::set_state(state new_state)
{
    m_state = new_state;
}

std::span<uint8_t> linux_async_io_request::data()
{
    return std::span<uint8_t>(
        reinterpret_cast<uint8_t*>(m_buffer) + m_buffer_data_offset,
        m_size
    );
}

linux_async_io_manager::linux_async_io_manager()
{
    struct statvfs stat_result;
    if (statvfs(".", &stat_result) == 0)
    {
        m_sector_size = stat_result.f_bsize;
    }
    else
    {
        m_sector_size = 4096;
    }

    m_thread = std::make_unique<std::thread>([this]() {
        worker_thread();
    });
}

linux_async_io_manager::~linux_async_io_manager()
{
    m_active = false;
    m_thread->join();
    m_thread = nullptr;

    for (auto& [path, fd] : m_file_handles)
    {
        close(fd);
    }
}

void linux_async_io_manager::worker_thread()
{
    db_set_thread_name("async io manager");

    while (m_active)
    {
        {
            std::unique_lock lock(m_request_mutex);

            // Insert new requests.
            m_pending_requests.insert(m_pending_requests.end(), m_new_requests.begin(), m_new_requests.end());
            m_new_requests.clear();

            // Free any pending buffers.
            for (void* ptr : m_pending_free)
            {
                free(ptr);
            }
            m_pending_free.clear();
        }

        {
            // If there are any pending requests and we have space in the queue, push it in.
            while (m_outstanding_requests.size() < k_ideal_queue_depth && !m_pending_requests.empty())
            {
                linux_async_io_request::ptr ret = m_pending_requests.front();
                m_pending_requests.erase(m_pending_requests.begin());

                if (start_request(ret))
                {
                    m_outstanding_requests.push_back(ret);
                }
            }

            // Check which requests have completed.
            for (auto iter = m_outstanding_requests.begin(); iter != m_outstanding_requests.end(); /*empty*/)
            {
                if (poll_request(*iter))
                {
                    // In future: If successful and we have decompression/decryption/etc options then queue
                    //            for those processes.

                    iter = m_outstanding_requests.erase(iter); // sloooow
                }
                else
                {
                    iter++;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

float linux_async_io_manager::get_current_bandwidth()
{
    std::unique_lock lock(m_request_mutex);

    return (float)m_bandwidth_average.get();
}

async_io_request::ptr linux_async_io_manager::request(const char* path, size_t offset, size_t size, async_io_request_options options)
{
    std::unique_lock lock(m_request_mutex);

    linux_async_io_request::ptr request = std::make_shared<linux_async_io_request>(this, path, offset, size, options);

    m_new_requests.push_back(request);

    return request;
}

void linux_async_io_manager::free_buffer(void* ptr)
{
    std::unique_lock lock(m_request_mutex);

    m_pending_free.push_back(ptr);
}

void* linux_async_io_manager::alloc_buffer(size_t size)
{
    void* ptr = nullptr;
    if (posix_memalign(&ptr, m_sector_size, size) != 0)
    {
        return nullptr;
    }
    return ptr;
}

bool linux_async_io_manager::start_request(linux_async_io_request::ptr request)
{
    memory_scope scope(memory_type::engine__async_io);

    request->m_file_descriptor = open_file(request->m_path.c_str());
    if (request->m_file_descriptor == -1)
    {
        request->set_state(linux_async_io_request::state::failed);
        return false;
    }

    // Size and offset of read must be sector aligned.
    request->m_read_offset = request->m_offset;
    request->m_read_size = request->m_size;

    // Align offset to sector start.
    if ((request->m_read_offset % m_sector_size) != 0)
    {
        size_t aligned_offset = (request->m_read_offset / m_sector_size) * m_sector_size;
        size_t delta = request->m_read_offset - aligned_offset;
        request->m_read_offset -= delta;
        request->m_read_size += delta;
        request->m_buffer_data_offset = delta;
    }

    // Align size to sector size.
    if ((request->m_read_size % m_sector_size) != 0)
    {
        request->m_read_size = math::round_up_multiple(request->m_read_size, m_sector_size);
    }

    // Request aligned buffer to store results.
    request->m_buffer = alloc_buffer(request->m_read_size);

    // And finally dispatch the read request.
    memset(&request->m_aiocb, 0, sizeof(request->m_aiocb));
    request->m_aiocb.aio_fildes = request->m_file_descriptor;
    request->m_aiocb.aio_offset = static_cast<off_t>(request->m_read_offset);
    request->m_aiocb.aio_buf = request->m_buffer;
    request->m_aiocb.aio_nbytes = request->m_read_size;

    request->m_start_time = get_seconds();

    profile_marker(profile_colors::task, "Read File");

    if (aio_read(&request->m_aiocb) == -1)
    {
        db_error(core, "Failed to run async read with error %i: %s", errno, request->m_path.c_str());
        request->set_state(linux_async_io_request::state::failed);
        return false;
    }

    request->set_state(linux_async_io_request::state::outstanding);

    return true;
}

bool linux_async_io_manager::poll_request(linux_async_io_request::ptr request)
{
    if (request->m_state == linux_async_io_request::state::completed)
    {
        return true;
    }

    int error = aio_error(&request->m_aiocb);
    if (error == EINPROGRESS)
    {
        return false;
    }

    if (error != 0)
    {
        db_error(core, "Failed to run async read with error %i: %s", error, request->m_path.c_str());
        request->set_state(linux_async_io_request::state::failed);
        return true;
    }

    ssize_t bytes_read = aio_return(&request->m_aiocb);
    if (bytes_read < 0 || static_cast<size_t>(bytes_read) < request->m_size)
    {
        db_error(core, "Failed to run async read, got %zi bytes expected at least %zi: %s", bytes_read, request->m_size, request->m_path.c_str());
        request->set_state(linux_async_io_request::state::failed);
    }
    else
    {
        // Track our average bandwidth.
        double elapsed = get_seconds() - request->m_start_time;

        {
            std::unique_lock lock(m_request_mutex);
            m_bandwidth_average.add((double)bytes_read, elapsed);
        }

        request->set_state(linux_async_io_request::state::completed);
    }

    return true;
}

int linux_async_io_manager::open_file(const char* path)
{
    if (auto iter = m_file_handles.find(path); iter != m_file_handles.end())
    {
        return iter->second;
    }

    profile_marker(profile_colors::task, "Open File");

    db_log(core, "Opening file for async io: %s", path);

    int fd = open(path, O_RDONLY | O_DIRECT);
    if (fd == -1)
    {
        db_error(core, "Failed to open file for async io with error %i: %s", errno, path);
        return fd;
    }

    m_file_handles[path] = fd;

    return fd;
}

}; // namespace workshop
