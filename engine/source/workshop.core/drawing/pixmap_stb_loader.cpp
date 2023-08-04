// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/drawing/pixmap_stb_loader.h"
#include "workshop.core/drawing/pixmap.h"
#include "workshop.core/debug/log.h"

#include "thirdparty/stb/stb_image.h"

namespace ws {

std::vector<std::unique_ptr<pixmap>> pixmap_stb_loader::load(const std::vector<char>& buffer)
{
    std::vector<std::unique_ptr<pixmap>> result_array;

    int width, height, channels;
    float* data = stbi_loadf_from_memory(reinterpret_cast<const stbi_uc*>(buffer.data()), static_cast<int>(buffer.size()), &width, &height, &channels, 4);
    if (data == nullptr)
    {
        db_log(core, "Failed to load pixmap using stb.");
        return {};
    }
    
    std::unique_ptr<pixmap> result;
    if (stbi_is_hdr_from_memory(reinterpret_cast<const stbi_uc*>(buffer.data()), static_cast<int>(buffer.size())))
    {
        result = std::make_unique<pixmap>(width, height, pixmap_format::R32G32B32A32_FLOAT);
    }
    else
    {
        result = std::make_unique<pixmap>(width, height, pixmap_format::R8G8B8A8);
    }

    for (size_t y = 0; y < height; y++)
    {
        for (size_t x = 0; x < width; x++)
        {
            color c(data[0], data[1], data[2], data[3]);
            result->set(x, y, c);
            data += 4;
        }
    }

    result_array.push_back(std::move(result));

    return result_array;
}

bool pixmap_stb_loader::save(pixmap& input, std::vector<char>& buffer)
{
    db_assert_message(false, "STB saving is not supported.");

    return false;
}

}; // namespace workshop
