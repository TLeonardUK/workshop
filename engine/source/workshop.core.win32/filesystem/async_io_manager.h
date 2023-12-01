// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/filesystem/async_io_manager.h"
#include "workshop.core.win32/utils/windows_headers.h"
#include "workshop.core/math/rolling_average.h"
#include "workshop.core/math/rolling_rate.h"

#include <filesystem>
#include <span>
#include <mutex>
#include <cstddef>
#include <unordered_map>

namespace ws {

// Win32 implementation of async io request.
class win32_async_io_request : public async_io_request
{
public:

    using ptr = std::shared_ptr<win32_async_io_request>;

    win32_async_io_request(const char* path, size_t offset, size_t size, async_io_request_options options);
    virtual ~win32_async_io_request();

    // Returns true once this request has completed.
    virtual bool is_complete() override;

    // Returns true if the request failed for any reason.
    virtual bool has_failed() override;

    // Gets the that was loaded from disk.
    virtual std::span<uint8_t> data() override;
  
private:
    friend class win32_async_io_manager;

    enum class state
    {
        pending,
        outstanding,
        completed,
        failed
    };

    void set_state(state new_state);

private:    
    std::string m_path;
    size_t m_offset;
    size_t m_size;
    async_io_request_options m_options;

    // Size and offset aligned to sector sizes.
    size_t m_read_offset;
    size_t m_read_size;

    state m_state = state::pending;

    size_t m_buffer_data_offset = 0;
    void* m_buffer = nullptr;

    double m_start_time = 0.0;

    HANDLE m_file_handle;
    OVERLAPPED m_overlapped;

};

// Win32 implementation of async io manager.
class win32_async_io_manager
    : public async_io_manager
{
public:
    win32_async_io_manager();
    ~win32_async_io_manager();

    // Gets the current IO bandwidth being used by all active requests in bytes per second.
    float get_current_bandwidth();

    // Starts a request to load the given block of data on the filesystem poinetd to by path
    // with the given offset and size.
    // 
    // No virtualization is performed on the path, this path is expected to be the 
    // raw on-disk path.
    async_io_request::ptr request(const char* path, size_t offset, size_t size, async_io_request_options options);

private:
    void worker_thread();

    bool start_request(win32_async_io_request::ptr request);
    bool poll_request(win32_async_io_request::ptr request);

    HANDLE open_file(const char* path);

private:    
    std::mutex m_request_mutex;

    HANDLE m_request_sempahore;
    
    std::list<win32_async_io_request::ptr> m_outstanding_requests;
    std::list<win32_async_io_request::ptr> m_pending_requests;

    std::unique_ptr<std::thread> m_thread;
    bool m_active = true;

    size_t m_sector_size = 0;

    std::unordered_map<std::string, HANDLE> m_file_handles;

    rolling_rate<double, 1.0> m_bandwidth_average;

    // Ideal number of requests to keep outstanding at any time to achieve peak
    // performance and keep memory usage in check.
    static inline const size_t k_ideal_queue_depth = 96;

};

}; // namespace workshop
