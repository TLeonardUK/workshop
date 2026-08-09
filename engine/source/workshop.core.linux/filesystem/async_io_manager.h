// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/filesystem/async_io_manager.h"
#include "workshop.core/math/rolling_average.h"
#include "workshop.core/math/rolling_rate.h"

#include <span>
#include <mutex>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <list>
#include <thread>

#include <aio.h>

namespace ws {

class linux_async_io_manager;

class linux_async_io_request : public async_io_request
{
public:
    using ptr = std::shared_ptr<linux_async_io_request>;

    linux_async_io_request(linux_async_io_manager* manager, const char* path, size_t offset, size_t size, async_io_request_options options);
    virtual ~linux_async_io_request();

    virtual bool is_complete() override;
    virtual bool has_failed() override;
    virtual std::span<uint8_t> data() override;

private:
    friend class linux_async_io_manager;

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

    linux_async_io_manager* m_manager;

    // Size and offset aligned to sector sizes.
    size_t m_read_offset;
    size_t m_read_size;

    state m_state = state::pending;

    size_t m_buffer_data_offset = 0;
    void* m_buffer = nullptr;

    double m_start_time = 0.0;

    int m_file_descriptor = -1;
    struct aiocb m_aiocb;

};

class linux_async_io_manager
    : public async_io_manager
{
public:
    linux_async_io_manager();
    ~linux_async_io_manager();

    // Gets the current IO bandwidth being used by all active requests in bytes per second.
    float get_current_bandwidth();

    // Starts a request to load the given block of data on the filesystem poinetd to by path
    // with the given offset and size.
    //
    // No virtualization is performed on the path, this path is expected to be the
    // raw on-disk path.
    async_io_request::ptr request(const char* path, size_t offset, size_t size, async_io_request_options options);

private:
    friend class linux_async_io_request;

    void free_buffer(void* ptr);
    void* alloc_buffer(size_t size);

    void worker_thread();

    bool start_request(linux_async_io_request::ptr request);
    bool poll_request(linux_async_io_request::ptr request);

    int open_file(const char* path);

private:
    std::mutex m_request_mutex;

    std::list<linux_async_io_request::ptr> m_outstanding_requests;
    std::list<linux_async_io_request::ptr> m_pending_requests;
    std::list<linux_async_io_request::ptr> m_new_requests;
    std::list<void*> m_pending_free;

    std::unique_ptr<std::thread> m_thread;
    bool m_active = true;

    size_t m_sector_size = 0;

    std::unordered_map<std::string, int> m_file_handles;

    rolling_rate<double, 1.0> m_bandwidth_average;

    // Ideal number of requests to keep outstanding at any time to achieve peak
    // performance and keep memory usage in check.
    static inline const size_t k_ideal_queue_depth = 96;

};

}; // namespace workshop
