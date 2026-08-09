// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/path_watcher.h"
#include "workshop.core/containers/string.h"

#include <array>
#include <unordered_map>

#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>

namespace ws {

class linux_path_watcher : public path_watcher
{
public:
    linux_path_watcher();
    virtual ~linux_path_watcher();

    bool init(const std::filesystem::path& path);
    void poll_changes();

    virtual bool get_next_change(event& out_event) override;

private:
    static constexpr uint32_t k_event_mask = IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_ATTRIB | IN_MOVE_SELF | IN_DELETE_SELF;

    bool add_watch(const std::filesystem::path& path);

    int m_inotify_fd = -1;
    std::unordered_map<int, std::filesystem::path> m_watch_paths;

    std::vector<event> m_pending_events;

};

linux_path_watcher::linux_path_watcher()
{
}

bool linux_path_watcher::add_watch(const std::filesystem::path& path)
{
    int wd = inotify_add_watch(m_inotify_fd, path.c_str(), k_event_mask);
    if (wd == -1)
    {
        db_log(core, "Failed to watch directory (errno=%i): %s", errno, path.string().c_str());
        return false;
    }

    m_watch_paths[wd] = path;

    std::error_code ec;
    for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(path, std::filesystem::directory_options::skip_permission_denied, ec))
    {
        if (entry.is_directory())
        {
            int sub_wd = inotify_add_watch(m_inotify_fd, entry.path().c_str(), k_event_mask);
            if (sub_wd != -1)
            {
                m_watch_paths[sub_wd] = entry.path();
            }
        }
    }

    return true;
}

bool linux_path_watcher::init(const std::filesystem::path& path)
{
    m_inotify_fd = inotify_init1(IN_NONBLOCK);
    if (m_inotify_fd == -1)
    {
        db_log(core, "Failed to create inotify instance (errno=%i)", errno);
        return false;
    }

    return add_watch(path);
}

linux_path_watcher::~linux_path_watcher()
{
    if (m_inotify_fd != -1)
    {
        close(m_inotify_fd);
        m_inotify_fd = -1;
    }
}

void linux_path_watcher::poll_changes()
{
    std::array<uint8_t, 16 * 1024> buffer;

    while (true)
    {
        ssize_t bytes_read = read(m_inotify_fd, buffer.data(), buffer.size());
        if (bytes_read <= 0)
        {
            break;
        }

        size_t offset = 0;
        while (offset < static_cast<size_t>(bytes_read))
        {
            inotify_event* info = reinterpret_cast<inotify_event*>(buffer.data() + offset);

            auto iter = m_watch_paths.find(info->wd);
            if (iter != m_watch_paths.end())
            {
                std::filesystem::path changed_path = iter->second;
                if (info->len > 0)
                {
                    changed_path /= info->name;
                }

                if ((info->mask & IN_IGNORED) != 0)
                {
                    m_watch_paths.erase(iter);
                }
                else
                {
                    event& evt = m_pending_events.emplace_back();
                    evt.type = event_type::modified;
                    evt.path = changed_path;

                    // Keep watching newly created subdirectories so the watch stays recursive.
                    if ((info->mask & IN_CREATE) != 0 && (info->mask & IN_ISDIR) != 0)
                    {
                        add_watch(changed_path);
                    }
                }
            }

            offset += sizeof(inotify_event) + info->len;
        }
    }
}

bool linux_path_watcher::get_next_change(event& out_event)
{
    poll_changes();

    if (!m_pending_events.empty())
    {
        out_event = m_pending_events.front();
        m_pending_events.erase(m_pending_events.begin());
        return true;
    }

    return false;
}

std::unique_ptr<path_watcher> watch_path(const std::filesystem::path& path)
{
    std::unique_ptr<linux_path_watcher> result = std::make_unique<linux_path_watcher>();
    if (!result->init(path))
    {
        return nullptr;
    }

    return result;
}

}; // namespace workshop
