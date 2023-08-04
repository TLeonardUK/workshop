// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/drawing/pixmap_png_loader.h"
#include "workshop.core/drawing/pixmap.h"
#include "workshop.core/debug/log.h"

#include "thirdparty/lodepng/lodepng.h"

namespace ws {

std::vector<std::unique_ptr<pixmap>> pixmap_png_loader::load(const std::vector<char>& buffer)
{
    unsigned char* image = nullptr;
    uint32_t width;
    uint32_t height;

    uint32_t error = lodepng_decode32(
        &image, 
        &width, 
        &height, 
        reinterpret_cast<const unsigned char*>(buffer.data()),
        buffer.size());

    if (error)
    {
        db_warning(core, "lodepng_decode32 failed with error %u: %s", error, lodepng_error_text(error));
        free(image);
        return {};
    }

    std::unique_ptr<pixmap> result = std::make_unique<pixmap>(std::span((uint8_t*)image, width * height * 4), width, height, pixmap_format::R8G8B8A8);

    free(image);

    std::vector<std::unique_ptr<pixmap>> result_array;
    result_array.push_back(std::move(result));

    return result_array;
}

bool pixmap_png_loader::save(pixmap& input, std::vector<char>& buffer)
{
    db_assert_message(input.get_format() == pixmap_format::R8G8B8A8, "Format is not valid for PNG format.");

    unsigned char* image = nullptr;
    size_t image_size = 0;
    uint32_t error = 0;

    if (input.get_format() == pixmap_format::R8G8B8A8)
    {
        error = lodepng_encode32(
            &image, 
            &image_size, 
            input.get_data().data(), 
            static_cast<unsigned int>(input.get_width()), 
            static_cast<unsigned int>(input.get_height()));
    }
    
    if (error)
    {
        db_warning(core, "lodepng_encode32 failed with error %u: %s", error, lodepng_error_text(error));
        free(image);
        return false;
    }

    buffer.resize(image_size);
    memcpy(buffer.data(), image, image_size);

    free(image);

    return true;
}

}; // namespace workshop
