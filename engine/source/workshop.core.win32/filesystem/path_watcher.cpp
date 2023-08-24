// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/path_watcher.h"
#include "workshop.core.win32/utils/windows_headers.h"
#include "workshop.core/containers/string.h"

#include <array>

namespace ws {

class win32_path_watcher : public path_watcher
{
public:
    win32_path_watcher();
    virtual ~win32_path_watcher();

    bool init(const std::filesystem::path& path);
    void poll_changes();

    virtual bool get_next_change(event& out_event) override;

private:
    HANDLE m_handle = INVALID_HANDLE_VALUE;
    std::array<uint8_t, 4096> m_buffer = {};
    OVERLAPPED m_overlapped;

    std::string m_path;

    std::vector<event> m_pending_events;

    DWORD m_event_mask = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY;

};

win32_path_watcher::win32_path_watcher()
{
    m_overlapped.hEvent = INVALID_HANDLE_VALUE;
}

bool win32_path_watcher::init(const std::filesystem::path& path)
{
    m_path = path.string();
    if (!string_ends_with(m_path, "\\"))
    {
        m_path.append("\\");
    }

    m_handle = CreateFile(
        path.string().c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr);

    if (m_handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    m_overlapped.hEvent = CreateEvent(NULL, FALSE, 0, NULL);
    if (m_overlapped.hEvent == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    bool success = ReadDirectoryChangesW(
        m_handle,
        m_buffer.data(),
        static_cast<DWORD>(m_buffer.size()),
        true,
        m_event_mask,
        nullptr,
        &m_overlapped,
        nullptr
    );

    return true;
}

win32_path_watcher::~win32_path_watcher()
{
    if (m_overlapped.hEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_overlapped.hEvent);
        m_overlapped.hEvent = INVALID_HANDLE_VALUE;
    }

    if (m_handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
}

void win32_path_watcher::poll_changes()
{
    DWORD ret = WaitForSingleObject(m_overlapped.hEvent, 0);
    if (ret == WAIT_OBJECT_0)
    {
        DWORD bytes_transferred;
        GetOverlappedResult(m_handle, &m_overlapped, &bytes_transferred, FALSE);

        // Read any events we recieved.
        FILE_NOTIFY_INFORMATION* info = (FILE_NOTIFY_INFORMATION*)m_buffer.data();
        while (true)
        {
            if (info->Action == FILE_ACTION_MODIFIED)
            {
                DWORD name_len = info->FileNameLength / sizeof(wchar_t);
                std::wstring relative_filename(info->FileName, name_len);
                std::string path = m_path + narrow_string(relative_filename);

                try
                {
                    if (std::filesystem::is_regular_file(path))
                    {
                        event& evt = m_pending_events.emplace_back();
                        evt.type = event_type::modified;
                        evt.path = path;
                    }
                }
                catch (std::filesystem::filesystem_error)
                {
                    // Failed to determine if valid file, so just skip it.
                }
            }

            if (info->NextEntryOffset) 
            {
                *(reinterpret_cast<uint8_t**>(&info)) += info->NextEntryOffset;
            }
            else 
            {
                break;
            }
        }

        // Start reading next events.
        bool success = ReadDirectoryChangesW(
            m_handle,
            m_buffer.data(),
            static_cast<DWORD>(m_buffer.size()),
            true,
            m_event_mask,
            nullptr,
            &m_overlapped,
            nullptr
        );

        if (!success)
        {
            db_error(core, "ReadDirectoryChangesW failed with result 0x%08x.", GetLastError());
        }
    }
}

bool win32_path_watcher::get_next_change(event& out_event)
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
    std::unique_ptr<win32_path_watcher> result = std::make_unique<win32_path_watcher>();
    if (!result->init(path))
    {
        return nullptr;
    }

    return result;
}

}; // namespace workshop
