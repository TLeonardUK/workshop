// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/async_io_manager.h"
#include "workshop.core/math/math.h"
#include "workshop.core/utils/time.h"
#include "workshop.core.win32/utils/windows_headers.h"
#include "workshop.core.win32/filesystem/async_io_manager.h"
#include "workshop.core/memory/memory_tracker.h"
#include "workshop.core/perf/profile.h"

#include <filesystem>
#include <span>
#include <mutex>
#include <cstddef>

namespace ws {

std::unique_ptr<async_io_manager> async_io_manager::create()
{
    return std::make_unique<win32_async_io_manager>();
}

win32_async_io_request::win32_async_io_request(win32_async_io_manager* manager, const char* path, size_t offset, size_t size, async_io_request_options options)
    : m_manager(manager)
    , m_path(path)
    , m_offset(offset)
    , m_size(size)
    , m_options(options)
{
}

win32_async_io_request::~win32_async_io_request()
{
    if (m_buffer)
    {
        m_manager->free_buffer(m_buffer);
        m_buffer = nullptr;
    }
}

bool win32_async_io_request::is_complete()
{
    return (m_state == state::completed || m_state == state::failed);
}

bool win32_async_io_request::has_failed()
{
    return (m_state == state::failed);
}

void win32_async_io_request::set_state(state new_state)
{
    m_state = new_state;
}

std::span<uint8_t> win32_async_io_request::data()
{
    return std::span<uint8_t>(
        reinterpret_cast<uint8_t*>(m_buffer) + m_buffer_data_offset,
        m_size
    );
}

win32_async_io_manager::win32_async_io_manager()
{
    DWORD bytes_per_second = 0;
    GetDiskFreeSpace("C:", nullptr, &bytes_per_second, nullptr, nullptr);
    m_sector_size = (size_t)bytes_per_second;

    m_request_sempahore = CreateSemaphore(nullptr, 0, std::numeric_limits<LONG>::max(), nullptr);

    m_thread = std::make_unique<std::thread>([this]() {
        worker_thread();
    });
}

win32_async_io_manager::~win32_async_io_manager()
{
    m_active = false;
    m_thread->join();
    m_thread = nullptr;

    for (auto& [path, handle] : m_file_handles)
    {
        CloseHandle(handle);
    }

    CloseHandle(m_request_sempahore);
}

void win32_async_io_manager::worker_thread()
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
                _aligned_free(ptr);
            }
            m_pending_free.clear();
        }

        {
            // If there are any pending requests and we have space in the queue, push it in.
            while (m_outstanding_requests.size() < k_ideal_queue_depth && !m_pending_requests.empty())
            {
                win32_async_io_request::ptr ret = m_pending_requests.front();
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

            //db_log(core, "Sleeping async io: %zi.", m_pending_requests.size());
        }

        // Wait for either the semaphore to be singled or for an IO alert.
        //WaitForSingleObjectEx(m_request_sempahore, INFINITE, true);
        Sleep(1);
    }
}

float win32_async_io_manager::get_current_bandwidth()
{
    std::unique_lock lock(m_request_mutex);

    return (float)m_bandwidth_average.get();
}

async_io_request::ptr win32_async_io_manager::request(const char* path, size_t offset, size_t size, async_io_request_options options)
{
    std::unique_lock lock(m_request_mutex);

    win32_async_io_request::ptr request = std::make_shared<win32_async_io_request>(this, path, offset, size, options);

    m_new_requests.push_back(request);
    ReleaseSemaphore(m_request_sempahore, 1, nullptr);

    return request;
}


void win32_async_io_manager::free_buffer(void* ptr)
{
    std::unique_lock lock(m_request_mutex);

    m_pending_free.push_back(ptr);
    ReleaseSemaphore(m_request_sempahore, 1, nullptr);
}

void* win32_async_io_manager::alloc_buffer(size_t size)
{
    return _aligned_malloc(size, m_sector_size);
}

bool win32_async_io_manager::start_request(win32_async_io_request::ptr request)
{
    memory_scope scope(memory_type::engine__async_io);

    request->m_file_handle = open_file(request->m_path.c_str());
    if (request->m_file_handle == INVALID_HANDLE_VALUE)
    {
        request->set_state(win32_async_io_request::state::failed);
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
    DWORD bytes_read = 0;

    memset(&request->m_overlapped, 0, sizeof(request->m_overlapped));
    request->m_overlapped.Offset = (DWORD)(request->m_read_offset & 0x00000000FFFFFFFF);
    request->m_overlapped.OffsetHigh = (DWORD)((request->m_read_offset >> 32) & 0x00000000FFFFFFFF);

    request->m_start_time = get_seconds();

    profile_marker(profile_colors::task, "Read File");

    if (!ReadFile(
        request->m_file_handle,
        request->m_buffer,
        (DWORD)request->m_read_size,
        &bytes_read,
        &request->m_overlapped))
    {
        if (GetLastError() != ERROR_IO_PENDING)
        {
            db_error(core, "Failed to run async read with error 0x%08x: %s", GetLastError(), request->m_path.c_str());
            request->set_state(win32_async_io_request::state::failed);
            return false;
        }
        else
        {
            request->set_state(win32_async_io_request::state::outstanding);
        }
    }
    else
    {
        request->set_state(win32_async_io_request::state::completed);
    }    

    return true;
}

bool win32_async_io_manager::poll_request(win32_async_io_request::ptr request)
{
    if (request->m_state == win32_async_io_request::state::completed)
    {
        return true;
    }

    DWORD bytes_read = 0;

    if (GetOverlappedResult(request->m_file_handle, 
        &request->m_overlapped,
        &bytes_read,
        FALSE))
    {
        if (bytes_read < request->m_size)
        {
            db_error(core, "Failed to run async read, got %zi bytes expected at least %zi: %s", (size_t)bytes_read, request->m_size, request->m_path.c_str());
            request->set_state(win32_async_io_request::state::failed);
        }
        else
        {
            // Track our average bandwidth.
            double elapsed = get_seconds() - request->m_start_time;

            {
                std::unique_lock lock(m_request_mutex);
                m_bandwidth_average.add((double)bytes_read, elapsed);
            }

            request->set_state(win32_async_io_request::state::completed);
        }
        return true;
    }
    // See if we've encountered an error.
    else if (GetLastError() != ERROR_IO_INCOMPLETE)
    {
        db_error(core, "Failed to run async read with error 0x%08x: %s", GetLastError(), request->m_path.c_str());
        request->set_state(win32_async_io_request::state::failed);
        return true;
    }

    return false;
}

HANDLE win32_async_io_manager::open_file(const char* path)
{
    if (auto iter = m_file_handles.find(path); iter != m_file_handles.end())
    {
        return iter->second;
    }

    profile_marker(profile_colors::task, "Open File");

    db_log(core,  "Opening file for async io: %s", path);

    HANDLE file = CreateFile(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
        nullptr);

    if (file == INVALID_HANDLE_VALUE)
    {
        db_error(core, "Failed to open file for async io with error 0x%08x: %s", GetLastError(), path);
        return file;
    }

    m_file_handles[path] = file;

    return file;   
}

}; // namespace workshop
