// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/filesystem/virtual_file_system_handler.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/containers/string.h"

#include <algorithm>

namespace ws {

namespace {

template <typename callback_type>
void iterate_handlers(const char* path, callback_type callback)
{
    std::string protocol;
    std::string filename;

    virtual_file_system::crack(path, protocol, filename);
    protocol = virtual_file_system::normalize(protocol.c_str());
    filename = virtual_file_system::normalize(filename.c_str());

    std::vector<virtual_file_system_handler*> handlers = virtual_file_system::get().get_handlers(protocol);
    for (virtual_file_system_handler* handler : handlers)
    {
        if (callback(protocol, filename, handler))
        {
            break;
        }
    }
}

};

virtual_file_system::virtual_file_system()
{
}

virtual_file_system::~virtual_file_system()
{
}

virtual_file_system::handler_id virtual_file_system::register_handler(const char* protocol, int32_t priority, std::unique_ptr<virtual_file_system_handler>&& handler)
{
    std::scoped_lock lock(m_handlers_mutex);
    
    registered_handle handle;
    handle.handler = std::move(handler);
    handle.priority = priority;
    handle.protocol = normalize(protocol);
    handle.id = m_next_handle_id++;
    m_handlers.push_back(std::move(handle));

    std::sort(m_handlers.begin(), m_handlers.end(), [](const registered_handle& a, const registered_handle& b) {
        return a.priority > b.priority;
    });

    return handle.id;
}

void virtual_file_system::unregister_handler(handler_id id)
{
    std::scoped_lock lock(m_handlers_mutex);

    auto iter = std::find_if(m_handlers.begin(), m_handlers.end(), [id](const registered_handle& handle){
        return handle.id == id;
    });

    if (iter != m_handlers.end())
    {
        m_handlers.erase(iter);
    }
}

std::string virtual_file_system::normalize(const char* path)
{
    std::string result = "";    

    // TODO: Might be worth sanitizing out and .. or . references, we
    //       shouldn't be using them in a vfs.

    char last_chr = '\0';
    for (size_t i = 0; path[i] != '\0'; i++)
    {
        char chr = path[i];
        
        // Swap backslashes for forward.
        if (chr == '\\')
        {
            chr = '/';
        }

        // Remove double slashes.
        if (chr == '/' && last_chr == '/')
        {
            continue;
        }

        // Lowercase the character.
        if (chr >= 'A' && chr <= 'Z')
        {
            chr = 'a' + (chr - 'A');
        }

        last_chr = chr;
        result += chr;
    }

    return result;
}

void virtual_file_system::crack(const char* path, std::string& protocol, std::string& filename)
{
    const char* colon_chr = strchr(path, ':');
    if (colon_chr == nullptr)
    {
        protocol = "";
        filename = path;
    }
    else
    {
        size_t pos = std::distance(path, colon_chr);
        protocol = std::string(path, pos);
        filename = colon_chr + 1;
    }
}

std::string virtual_file_system::replace_protocol(const char* path, const char* new_protocol)
{
    std::string protocol, filename;
    crack(path, protocol, filename);
    return string_format("%s:%s", new_protocol, filename.c_str());
}

std::string virtual_file_system::get_parent(const char* path)
{
    const char* colon_chr = strrchr(path, '/');
    if (colon_chr == nullptr)
    {
        return path;
    }
    else
    {
        size_t pos = std::distance(path, colon_chr);
        return std::string(path, pos);
    }
}

std::string virtual_file_system::get_extension(const char* path)
{
    const char* colon_chr = strrchr(path, '.');
    if (colon_chr == nullptr)
    {
        return "";
    }
    else
    {
        size_t pos = std::distance(path, colon_chr);
        return std::string(path + pos);
    }
}

std::vector<virtual_file_system_handler*> virtual_file_system::get_handlers(const std::string& protocol)
{
    std::scoped_lock lock(m_handlers_mutex);

    std::vector<virtual_file_system_handler*> result;

    for (registered_handle& handle : m_handlers)
    {
        if (handle.protocol == protocol)
        {
            result.push_back(handle.handler.get());
        }
    }

    return result;
}

std::unique_ptr<stream> virtual_file_system::open(const char* path, bool forWriting)
{
    std::unique_ptr<stream> result;

    iterate_handlers(path, [&result, &forWriting](const std::string& protocol, const std::string& filename, virtual_file_system_handler* handler) -> bool {
        if (auto stream = handler->open(filename.c_str(), forWriting))
        {
            result = std::move(stream);
            return true;
        }
        return false;
    });

    return std::move(result);
}

virtual_file_system_path_type virtual_file_system::type(const char* path)
{
    virtual_file_system_path_type result = virtual_file_system_path_type::non_existant;
    
    iterate_handlers(path, [&result](const std::string& protocol, const std::string& filename, virtual_file_system_handler* handler) -> bool {
        result = handler->type(filename.c_str());
        return (result != virtual_file_system_path_type::non_existant);
    });

    return result;
}

bool virtual_file_system::exists(const char* path)
{
    return type(path) != virtual_file_system_path_type::non_existant;
}

bool virtual_file_system::create_directory(const char* path)
{
    bool result = false;

    iterate_handlers(path, [&result](const std::string& protocol, const std::string& filename, virtual_file_system_handler* handler) -> bool {
        result = handler->create_directory(filename.c_str());
        return result;
    });

    return result;
}

bool virtual_file_system::rename(const char* source, const char* destination)
{
    bool result = false;

    std::string destination_protocol, destination_filename;
    std::string source_protocol, source_filename;

    crack(destination, destination_protocol, destination_filename);
    destination_protocol = normalize(destination_protocol.c_str());
    destination_filename = normalize(destination_filename.c_str());

    crack(source, source_protocol, source_filename);
    source_protocol = normalize(source_protocol.c_str());
    source_filename = normalize(source_filename.c_str());

    if (source_protocol != destination_protocol)
    {
        // Must use the same protocol to have atomic renames.
        return false;
    }

    iterate_handlers(source, [&result, &destination_filename](const std::string& protocol, const std::string& filename, virtual_file_system_handler* handler) -> bool {
        result = handler->rename(filename.c_str(), destination_filename.c_str());
        return result;
    });

    return result;
}

bool virtual_file_system::copy(const char* source_f, const char* destination_f)
{
    std::unique_ptr<stream> source = open(source_f, false);
    std::unique_ptr<stream> dest = open(destination_f, true);
    if (!source || !dest)
    {
        return false;
    }
    if (!source->copy_to(*dest))
    {
        return false;
    }
    return true;
}

bool virtual_file_system::remove(const char* path)
{
    bool result = false;
    
    iterate_handlers(path, [&result](const std::string& protocol, const std::string& filename, virtual_file_system_handler* handler) -> bool {
        result = handler->remove(filename.c_str());
        return result;
    });

    return result;
}

bool virtual_file_system::modified_time(const char* path, virtual_file_system_time_point& timepoint)
{
    bool result = false;
    
    iterate_handlers(path, [&result, &timepoint](const std::string& protocol, const std::string& filename, virtual_file_system_handler* handler) -> bool {
        result = handler->modified_time(filename.c_str(), timepoint);
        return result;
    });

    return result;
}

std::vector<std::string> virtual_file_system::list(const char* path, virtual_file_system_path_type type)
{
    std::vector<std::string> result;
    
    iterate_handlers(path, [&result, &type](const std::string& protocol, const std::string& filename, virtual_file_system_handler* handler) -> bool {
        std::vector<std::string> files = handler->list(filename.c_str(), type);
        for (std::string& file : files)
        {
            std::string file_with_protocol = protocol + ":" + file;
            if (std::find(result.begin(), result.end(), file_with_protocol) == result.end())
            {
                result.push_back(file_with_protocol);
            }
        }
        return false;
    });

    return result;
}

std::unique_ptr<virtual_file_system_watcher> virtual_file_system::watch(const char* path, virtual_file_system_watcher::callback_t callback)
{
    std::unique_ptr<virtual_file_system_watcher_compound> result = std::make_unique<virtual_file_system_watcher_compound>();

    std::string protocol;
    std::string filename;
    virtual_file_system::crack(path, protocol, filename);
    protocol = virtual_file_system::normalize(protocol.c_str());

    // Handlers return relative paths, they don't include the alias so we have a special callback here to just prefix
    // the alias and pass it on to the real callback.
    virtual_file_system_watcher::callback_t alias_callback = [callback, protocol](const char* callback_path)
    {
        callback((protocol + ":" + callback_path).c_str());
    };

    iterate_handlers(path, [&result, &path, &alias_callback](const std::string& protocol, const std::string& filename, virtual_file_system_handler* handler) -> bool {

        std::unique_ptr<virtual_file_system_watcher> handler_watch = handler->watch(filename.c_str(), alias_callback);
        if (handler_watch)
        {
            result->watchers.push_back(std::move(handler_watch));
        }

        return false;

    });

    return result;
}

void virtual_file_system::raise_watch_events()
{
    for (registered_handle& handle : m_handlers)
    {
        handle.handler->raise_watch_events();
    }
}

}; // namespace workshop
