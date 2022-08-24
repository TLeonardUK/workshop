// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/containers/json.h"

namespace ws {

nlohmann::json* json_get_parent_by_name(nlohmann::json& input, const std::string& path, bool create_if_doesnt_exist)
{
    std::string fragment = "";
    std::string remaining = path;

    nlohmann::json* node = &input;

    while (true)
    {
        size_t pos = remaining.find("/");
        if (pos == std::string::npos)
        {
            break;
        }

        fragment = remaining.substr(0, pos);
        remaining = remaining.substr(pos + 1);

        if (!node->contains(fragment))
        {
            if (create_if_doesnt_exist)
            {
                (*node)[fragment] = nlohmann::json::object();
            }
            else
            {
                return nullptr;
            }
        }

        node = &node->at(fragment);
    }

    return node;
}

std::string json_get_node_name(const std::string& path)
{
    size_t pos = path.find_last_of("/");
    if (pos == std::string::npos)
    {
        return path;
    }

    return path.substr(pos + 1);
}

}; // namespace workshop
