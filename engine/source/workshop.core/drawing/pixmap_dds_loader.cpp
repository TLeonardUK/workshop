// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/drawing/pixmap_dds_loader.h"
#include "workshop.core/drawing/pixmap.h"
#include "workshop.core/debug/log.h"

#include "thirdparty/dds-ktx/dds-ktx.h"

namespace ws {

std::vector<std::unique_ptr<pixmap>> pixmap_dds_loader::load(const std::vector<char>& buffer)
{
    std::vector<std::unique_ptr<pixmap>> result_array;

    ddsktx_texture_info info = {0};
    ddsktx_error error;
    if (!ddsktx_parse(&info, buffer.data(), (int)buffer.size(), &error)) 
    {
        db_log(core, "Failed to load dds file, error: %s", error.msg);
        return {};
    }

    ddsktx_sub_data sub_data;
    ddsktx_get_sub(&info, &sub_data, buffer.data(), buffer.size(), 0, 0, 0);

    pixmap_format direct_format = pixmap_format::COUNT;
    size_t block_size = 1;

    switch (info.format)
    {
    case DDSKTX_FORMAT_RGBA8: direct_format = pixmap_format::R8G8B8A8; break;
    case DDSKTX_FORMAT_RGBA8S: direct_format = pixmap_format::R8G8B8A8_SIGNED; break;
    case DDSKTX_FORMAT_RG8: direct_format = pixmap_format::R8G8; break;
    case DDSKTX_FORMAT_RG8S: direct_format = pixmap_format::R8G8_SIGNED; break;
    case DDSKTX_FORMAT_R32F: direct_format = pixmap_format::R32_FLOAT; break;
    case DDSKTX_FORMAT_R8: direct_format = pixmap_format::R8; break;
    case DDSKTX_FORMAT_BC1: direct_format = pixmap_format::BC1; block_size = 4; break;
    case DDSKTX_FORMAT_BC3: direct_format = pixmap_format::BC3; block_size = 4; break;
    case DDSKTX_FORMAT_BC4: direct_format = pixmap_format::BC4; block_size = 4; break;
    case DDSKTX_FORMAT_BC5: direct_format = pixmap_format::BC5; block_size = 4; break;
    case DDSKTX_FORMAT_BC7: direct_format = pixmap_format::BC7; block_size = 4; break;
    }

    // If we have a direct format we can just return it directly.
    if (direct_format != pixmap_format::COUNT)
    {
        std::unique_ptr<pixmap> result = std::make_unique<pixmap>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(sub_data.buff), sub_data.size_bytes), sub_data.width, sub_data.height, direct_format);

        result_array.push_back(result->convert(pixmap_format::R8G8B8A8));
        return result_array;
    }

    // Otherwise we need to do some conversions.
    std::unique_ptr<pixmap> result = std::make_unique<pixmap>(info.width, info.height, pixmap_format::R8G8B8A8);

    for (size_t y = 0; y < info.height; y++)
    {
        for (size_t x = 0; x < info.width; x++)
        {
            color pixel(1.0f, 1.0f, 1.0f, 1.0f);

            const uint8_t* pixel_data = 
                reinterpret_cast<const uint8_t*>(sub_data.buff) + 
                (y * sub_data.row_pitch_bytes) + 
                (x * info.bpp);

            switch (info.format)
            {
            case DDSKTX_FORMAT_A8:
                {
                    pixel.a = pixel_data[0] / 255.0f;
                    break;
                }
            case DDSKTX_FORMAT_RGB8: 
                {
                    pixel.r = pixel_data[0] / 255.0f;
                    pixel.g = pixel_data[1] / 255.0f;
                    pixel.b = pixel_data[2] / 255.0f;
                    break;
                }
            case DDSKTX_FORMAT_BGRA8:
                {
                    pixel.b = pixel_data[0] / 255.0f;
                    pixel.g = pixel_data[1] / 255.0f;
                    pixel.r = pixel_data[2] / 255.0f;
                    pixel.a = pixel_data[3] / 255.0f;
                    break;
                }
            default:
                {
                    db_log(core, "Failed to load dds file, format is not supported.");
                    return {};
                }
            }

            result->set(x, y, pixel);
        }
    }

    result_array.push_back(result->convert(pixmap_format::R8G8B8A8));
    return result_array;
}

bool pixmap_dds_loader::save(pixmap& input, std::vector<char>& buffer)
{
    db_assert_message(false, "DDS saving is not supported.");

    return false;
}

}; // namespace workshop
