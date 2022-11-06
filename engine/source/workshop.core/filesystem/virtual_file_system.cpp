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
    std::string protocol;
    std::string filename;

    crack(path, protocol, filename);
    protocol = normalize(protocol.c_str());
    filename = normalize(filename.c_str());

    std::vector<virtual_file_system_handler*> handlers = get_handlers(protocol);
    for (virtual_file_system_handler* handler : handlers)
    {
        if (auto stream = handler->open(filename.c_str(), forWriting))
        {
            return std::move(stream);
        }
    }    

    return nullptr;
}

virtual_file_system_path_type virtual_file_system::type(const char* path)
{
    std::string protocol;
    std::string filename;

    crack(path, protocol, filename);
    protocol = normalize(protocol.c_str());
    filename = normalize(filename.c_str());

    std::vector<virtual_file_system_handler*> handlers = get_handlers(protocol);
    for (virtual_file_system_handler* handler : handlers)
    {
        virtual_file_system_path_type result = handler->type(filename.c_str());
        if (result != virtual_file_system_path_type::non_existant)
        {
            return result;
        }
    }

    return virtual_file_system_path_type::non_existant;
}

bool virtual_file_system::remove(const char* path)
{
    std::string protocol;
    std::string filename;

    crack(path, protocol, filename);
    protocol = normalize(protocol.c_str());
    filename = normalize(filename.c_str());

    std::vector<virtual_file_system_handler*> handlers = get_handlers(protocol);
    for (virtual_file_system_handler* handler : handlers)
    {
        if (handler->remove(filename.c_str()))
        {
            return true;
        }
    }

    return false;
}

bool virtual_file_system::modified_time(const char* path, virtual_file_system_time_point& timepoint)
{
    std::string protocol;
    std::string filename;

    crack(path, protocol, filename);
    protocol = normalize(protocol.c_str());
    filename = normalize(filename.c_str());

    std::vector<virtual_file_system_handler*> handlers = get_handlers(protocol);
    for (virtual_file_system_handler* handler : handlers)
    {
        if (handler->modified_time(filename.c_str(), timepoint))
        {
            return true;
        }
    }

    return false;
}

std::vector<std::string> virtual_file_system::list(const char* path, virtual_file_system_path_type type)
{
    std::string protocol;
    std::string filename;

    crack(path, protocol, filename);
    protocol = normalize(protocol.c_str());
    filename = normalize(filename.c_str());

    std::vector<std::string> result;

    std::vector<virtual_file_system_handler*> handlers = get_handlers(protocol);
    for (virtual_file_system_handler* handler : handlers)
    {
        std::vector<std::string> files = handler->list(filename.c_str(), type);
        for (std::string& file : files)
        {
            if (std::find(result.begin(), result.end(), file) == result.end())
            {
                result.push_back(file);
            }
        }
    }

    return result;
}

}; // namespace workshop
