// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/drawing/pixmap.h"
#include "workshop.core/drawing/pixmap_png_loader.h"
#include "workshop.core/drawing/pixmap_dds_loader.h"
#include "workshop.core/drawing/pixmap_stb_loader.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/math/math.h"
#include "workshop.core/async/async.h"
#include "workshop.core/utils/time.h"
#include "thirdparty/bc7enc/rgbcx.h"
#include "thirdparty/bc7enc/bc7enc.h"
#include "thirdparty/bc7enc/bc7decomp.h"
#include "thirdparty/compressonator/cmp_core/source/cmp_core.h"
#include "thirdparty/compressonator/cmp_core/shaders/bcn_common_api.h"

#include <vector>
#include <filesystem>

// If set then individual rows in a pixmap will be encoded in parallel to make better use of
// cpu cores.
#define PIXMAP_PARALLEL_ENCODE 1

namespace ws {

// TODO: We should probably find somewhere a bit more appropriate for this.
struct bc7enc_initializer
{
    bc7enc_initializer()
    {
        rgbcx::init();
        bc7enc_compress_block_init();
    }
} g_bc7enc_initializer;

pixmap_format_metrics get_pixmap_format_metrics(pixmap_format value)
{
    static std::array<pixmap_format_metrics, static_cast<int>(pixmap_format::COUNT)> conversion = {
        pixmap_format_metrics { .pixel_size=16, .channels={0, 1, 2, 3}, .channel_size=4, .channel_format=pixmap_channel_format::t_float, .is_mutable=true },                   // R32G32B32A32_FLOAT,
        pixmap_format_metrics { .pixel_size=16, .channels={0, 1, 2, 3}, .channel_size=4, .channel_format=pixmap_channel_format::t_signed_int, .is_mutable=true },              // R32G32B32A32_SIGNED,
        pixmap_format_metrics { .pixel_size=16, .channels={0, 1, 2, 3}, .channel_size=4, .channel_format=pixmap_channel_format::t_unsigned_int, .is_mutable=true },            // R32G32B32A32,
        
        pixmap_format_metrics { .pixel_size=12, .channels={0, 1, 2}, .channel_size=4, .channel_format=pixmap_channel_format::t_float, .is_mutable=true },                      // R32G32B32_FLOAT,
        pixmap_format_metrics { .pixel_size=12, .channels={0, 1, 2}, .channel_size=4, .channel_format=pixmap_channel_format::t_signed_int, .is_mutable=true },                 // R32G32B32_SIGNED,
        pixmap_format_metrics { .pixel_size=12, .channels={0, 1, 2}, .channel_size=4, .channel_format=pixmap_channel_format::t_unsigned_int, .is_mutable=true },               // R32G32B32,
        
        pixmap_format_metrics { .pixel_size=8, .channels={0, 1}, .channel_size=4, .channel_format=pixmap_channel_format::t_float, .is_mutable=true },                          // R32G32_FLOAT,
        pixmap_format_metrics { .pixel_size=8, .channels={0, 1}, .channel_size=4, .channel_format=pixmap_channel_format::t_signed_int, .is_mutable=true },                     // R32G32_SIGNED,
        pixmap_format_metrics { .pixel_size=8, .channels={0, 1}, .channel_size=4, .channel_format=pixmap_channel_format::t_unsigned_int, .is_mutable=true },                   // R32G32,
        
        pixmap_format_metrics { .pixel_size=4, .channels={0}, .channel_size=4, .channel_format=pixmap_channel_format::t_float, .is_mutable=true },                             // R32_FLOAT,
        pixmap_format_metrics { .pixel_size=4, .channels={0}, .channel_size=4, .channel_format=pixmap_channel_format::t_signed_int, .is_mutable=true },                        // R32_SIGNED,
        pixmap_format_metrics { .pixel_size=4, .channels={0}, .channel_size=4, .channel_format=pixmap_channel_format::t_unsigned_int, .is_mutable=true },                      // R32,
        
        pixmap_format_metrics { .pixel_size=8, .channels={0, 1, 2, 3}, .channel_size=2, .channel_format=pixmap_channel_format::t_float, .is_mutable=true },                    // R16G16B16A16_FLOAT,
        pixmap_format_metrics { .pixel_size=8, .channels={0, 1, 2, 3}, .channel_size=2, .channel_format=pixmap_channel_format::t_signed_int, .is_mutable=true },               // R16G16B16A16_SIGNED,
        pixmap_format_metrics { .pixel_size=8, .channels={0, 1, 2, 3}, .channel_size=2, .channel_format=pixmap_channel_format::t_unsigned_int, .is_mutable=true },             // R16G16B16A16,
        
        pixmap_format_metrics { .pixel_size=4, .channels={0, 1}, .channel_size=2, .channel_format=pixmap_channel_format::t_float, .is_mutable=true },                          // R16G16_FLOAT,
        pixmap_format_metrics { .pixel_size=4, .channels={0, 1}, .channel_size=2, .channel_format=pixmap_channel_format::t_signed_int, .is_mutable=true },                     // R16G16_SIGNED,
        pixmap_format_metrics { .pixel_size=4, .channels={0, 1}, .channel_size=2, .channel_format=pixmap_channel_format::t_unsigned_int, .is_mutable=true },                   // R16G16,
        
        pixmap_format_metrics { .pixel_size=2, .channels={0}, .channel_size=2, .channel_format=pixmap_channel_format::t_float, .is_mutable=true },                             // R16_FLOAT,
        pixmap_format_metrics { .pixel_size=2, .channels={0}, .channel_size=2, .channel_format=pixmap_channel_format::t_signed_int, .is_mutable=true },                        // R16_SIGNED,
        pixmap_format_metrics { .pixel_size=2, .channels={0}, .channel_size=2, .channel_format=pixmap_channel_format::t_unsigned_int, .is_mutable=true },                      // R16,
        
        pixmap_format_metrics { .pixel_size=4, .channels={0, 1, 2, 3}, .channel_size=1, .channel_format=pixmap_channel_format::t_signed_int, .is_mutable=true },               // R8G8B8A8_SIGNED,
        pixmap_format_metrics { .pixel_size=4, .channels={0, 1, 2, 3}, .channel_size=1, .channel_format=pixmap_channel_format::t_unsigned_int, .is_mutable=true },             // R8G8B8A8,
        
        pixmap_format_metrics { .pixel_size=2, .channels={0, 1}, .channel_size=1, .channel_format=pixmap_channel_format::t_signed_int, .is_mutable=true },                     // R8G8,
        pixmap_format_metrics { .pixel_size=2, .channels={0, 1}, .channel_size=1, .channel_format=pixmap_channel_format::t_unsigned_int, .is_mutable=true },                   // R8G8_SIGNED,
        
        pixmap_format_metrics { .pixel_size=1, .channels={0}, .channel_size=1, .channel_format=pixmap_channel_format::t_signed_int, .is_mutable=true },                        // R8,
        pixmap_format_metrics { .pixel_size=1, .channels={0}, .channel_size=1, .channel_format=pixmap_channel_format::t_unsigned_int, .is_mutable=true },                      // R8_SIGNED,
        
        pixmap_format_metrics { .pixel_size=0, .channels={}, .channel_size=0, .channel_format=pixmap_channel_format::t_signed_int, .is_mutable=false, .block_size=4, .encoded_block_size=8 },         // BC1,
        pixmap_format_metrics { .pixel_size=0, .channels={}, .channel_size=0, .channel_format=pixmap_channel_format::t_signed_int, .is_mutable=false, .block_size=4, .encoded_block_size=16 },        // BC3,
        pixmap_format_metrics { .pixel_size=0, .channels={}, .channel_size=0, .channel_format=pixmap_channel_format::t_signed_int, .is_mutable=false, .block_size=4, .encoded_block_size=8 },         // BC4,
        pixmap_format_metrics { .pixel_size=0, .channels={}, .channel_size=0, .channel_format=pixmap_channel_format::t_signed_int, .is_mutable=false, .block_size=4, .encoded_block_size=16 },        // BC5,
        pixmap_format_metrics { .pixel_size=0, .channels={}, .channel_size=0, .channel_format=pixmap_channel_format::t_signed_int, .is_mutable=false, .block_size=4, .encoded_block_size=16 },        // BC7,
        
        pixmap_format_metrics { .pixel_size=0, .channels={}, .channel_size=0, .channel_format=pixmap_channel_format::t_signed_int, .is_mutable=false, .block_size=4, .encoded_block_size=16 },        // BC6H_SF16,
        pixmap_format_metrics { .pixel_size=0, .channels={}, .channel_size=0, .channel_format=pixmap_channel_format::t_signed_int, .is_mutable=false, .block_size=4, .encoded_block_size=16 },        // BC6H_UF16,
    };

    if (size_t index = static_cast<int>(value); math::in_range(index, 0llu, conversion.size()))
    {
        return conversion[index];
    }
    else
    {
        db_assert_message(false, "Out of bounds conversion of pixmap_format: %llu", index);
        return conversion[0];
    }
}

pixmap::pixmap(std::span<const uint8_t> data, size_t width, size_t height, pixmap_format format)
    : m_width(width)
    , m_height(height)
    , m_format(format)
    , m_format_metrics(get_pixmap_format_metrics(m_format))
    , m_data(data.data(), data.data() + data.size_bytes())
{
    m_row_stride = m_width * m_format_metrics.pixel_size;
}

pixmap::pixmap(size_t width, size_t height, pixmap_format format)
    : m_width(width)
    , m_height(height)
    , m_format(format)
    , m_format_metrics(get_pixmap_format_metrics(m_format))
{
    m_row_stride = m_width * m_format_metrics.pixel_size;
    m_data.resize(m_width * m_height * m_format_metrics.pixel_size, 0);
}

void pixmap::set(size_t x, size_t y, const color& color)
{
    db_assert_message(m_format_metrics.is_mutable, "Attempting to write to non-mutable image format, this is not supported.");

    size_t pixel_size = m_format_metrics.pixel_size;
    uint8_t* data = m_data.data() + (y * m_row_stride) + (x * pixel_size);

    // Fast path for the most common formats we will be modifying in.
    if (m_format == pixmap_format::R32G32B32A32_FLOAT)
    {
        memcpy(data, &color.r, 16);
        return;
    }
    else if (m_format == pixmap_format::R8G8B8A8)
    {
        static constexpr float k_conversion_factor = 1.0f / 255.0f;

        uint8_t raw_data[4] = {
            static_cast<uint8_t>(color.r * 255),
            static_cast<uint8_t>(color.g * 255),
            static_cast<uint8_t>(color.b * 255),
            static_cast<uint8_t>(color.a * 255)
        };

        memcpy(data, raw_data, 4);
        return;
    }
    
    // Backup code to handle unusual formats/swizzle patterns.
    size_t channel_source_offset = 0;
    for (size_t channel : m_format_metrics.channels)
    {
        uint8_t* channel_data = data + channel_source_offset;

        switch (m_format_metrics.channel_format)
        {
        case pixmap_channel_format::t_unsigned_int:
            {
                if (m_format_metrics.channel_size == 4)
                {
                    *reinterpret_cast<uint32_t*>(channel_data) = static_cast<uint32_t>(color[channel] * std::numeric_limits<uint32_t>::max());
                }
                else if (m_format_metrics.channel_size == 2)
                {
                    *reinterpret_cast<uint16_t*>(channel_data) = static_cast<uint16_t>(color[channel] * std::numeric_limits<uint16_t>::max());
                }
                else if (m_format_metrics.channel_size == 1)
                {
                    *reinterpret_cast<uint8_t*>(channel_data) = static_cast<uint8_t>(color[channel] * std::numeric_limits<uint8_t>::max());
                }
                else
                {
                    db_assert_message(false, "Invalid channel size for integer format: %zi", m_format_metrics.channel_size);
                }
                break;
            }
        case pixmap_channel_format::t_signed_int:
            {
                if (m_format_metrics.channel_size == 4)
                {
                    *reinterpret_cast<int32_t*>(channel_data) = static_cast<int32_t>(color[channel] * std::numeric_limits<int32_t>::max());
                }
                else if (m_format_metrics.channel_size == 2)
                {
                    *reinterpret_cast<int16_t*>(channel_data) = static_cast<int16_t>(color[channel] * std::numeric_limits<int16_t>::max());
                }
                else if (m_format_metrics.channel_size == 1)
                {
                    *reinterpret_cast<int8_t*>(channel_data) = static_cast<int8_t>(color[channel] * std::numeric_limits<int8_t>::max());
                }
                else
                {
                    db_assert_message(false, "Invalid channel size for integer format: %zi", m_format_metrics.channel_size);
                }
                break;
            }
        case pixmap_channel_format::t_float:
            {
                if (m_format_metrics.channel_size == 4)
                {
                    *reinterpret_cast<float*>(channel_data) = color[channel];
                }
                else if (m_format_metrics.channel_size == 2)
                {
                    *reinterpret_cast<int16_t*>(channel_data) = math::to_float16(color[channel]);
                }
                else
                {
                    db_assert_message(false, "Invalid channel size for floating point format: %zi", m_format_metrics.channel_size);
                }
                break;
            }
        }

        channel_source_offset += m_format_metrics.channel_size;
    }
}

color pixmap::get(size_t x, size_t y) const
{
    db_assert_message(m_format_metrics.is_mutable, "Attempting to read from non-mutable image format, this is not supported.");

    size_t pixel_size = m_format_metrics.pixel_size;
    const uint8_t* data = m_data.data() + (y * m_row_stride) + (x * pixel_size);

    color result(0.0f, 0.0f, 0.0f, 0.0f);

    // Fast path for the most common formats we will be modifying in.
    if (m_format == pixmap_format::R32G32B32A32_FLOAT)
    {
        memcpy(&result.r, data, 16);
        return result;
    }
    else if (m_format == pixmap_format::R8G8B8A8)
    {
        static constexpr float k_conversion_factor = 1.0f / 255.0f;

        uint8_t raw_data[4];
        memcpy(raw_data, data, 4);
        result.r = raw_data[0] * k_conversion_factor;
        result.g = raw_data[1] * k_conversion_factor;
        result.b = raw_data[2] * k_conversion_factor;
        result.a = raw_data[3] * k_conversion_factor;
        return result;
    }

    // Backup code to handle unusual formats/swizzle patterns.
    size_t channel_source_offset = 0;
    for (size_t channel : m_format_metrics.channels)
    {
        const uint8_t* channel_data = data + channel_source_offset;

        switch (m_format_metrics.channel_format)
        {
        case pixmap_channel_format::t_unsigned_int:
            {
                if (m_format_metrics.channel_size == 4)
                {
                    result[channel] = static_cast<float>(*reinterpret_cast<const uint32_t*>(channel_data)) / std::numeric_limits<uint32_t>::max();
                }
                else if (m_format_metrics.channel_size == 2)
                {
                    result[channel] = static_cast<float>(*reinterpret_cast<const uint16_t*>(channel_data)) / std::numeric_limits<uint16_t>::max();
                }
                else if (m_format_metrics.channel_size == 1)
                {
                    result[channel] = static_cast<float>(*reinterpret_cast<const uint8_t*>(channel_data)) / std::numeric_limits<uint8_t>::max();
                }
                else
                {
                    db_assert_message(false, "Invalid channel size for integer format: %zi", m_format_metrics.channel_size);
                }
                break;
            }
        case pixmap_channel_format::t_signed_int:
            {
                if (m_format_metrics.channel_size == 4)
                {
                    result[channel] = static_cast<float>(*reinterpret_cast<const int32_t*>(channel_data)) / std::numeric_limits<int32_t>::max();
                }
                else if (m_format_metrics.channel_size == 2)
                {
                    result[channel] = static_cast<float>(*reinterpret_cast<const int16_t*>(channel_data)) / std::numeric_limits<int16_t>::max();
                }
                else if (m_format_metrics.channel_size == 1)
                {
                    result[channel] = static_cast<float>(*reinterpret_cast<const int8_t*>(channel_data)) / std::numeric_limits<int8_t>::max();
                }
                else
                {
                    db_assert_message(false, "Invalid channel size for integer format: %zi", m_format_metrics.channel_size);
                }
                break;
            }
        case pixmap_channel_format::t_float:
            {
                if (m_format_metrics.channel_size == 4)
                {
                    result[channel] = *reinterpret_cast<const float*>(channel_data);
                }
                else if (m_format_metrics.channel_size == 2)
                {
                    result[channel] = math::from_float16(*reinterpret_cast<const uint16_t*>(channel_data));
                }
                else
                {
                    db_assert_message(false, "Invalid channel size for floating point format: %zi", m_format_metrics.channel_size);
                }
                break;
            }
        }

        channel_source_offset += m_format_metrics.channel_size;
    }

    return result;
}

color pixmap::sample(float x, float y, pixmap_filter filter)
{
    float pixel_half_width = (1.0f / m_width) * 0.5f;
    float pixel_half_height = (1.0f / m_height) * 0.5f;

    // Note: This would be a lot fast if we implemented things like resize as seperate
    //       horizontal and vertical passes.

    // Some useful reference for different filters:
    // http://bertolami.com/index.php?engine=blog&content=posts&detail=inside-imagine-kernels

    switch (filter)
    {
    case pixmap_filter::nearest_neighbour:
        {
            size_t sample_x = static_cast<size_t>(floorf(x));
            size_t sample_y = static_cast<size_t>(floorf(y));
            float delta_x = x - sample_x;
            float delta_y = y - sample_y;

            size_t src_x = std::clamp((delta_x < 0.5f ? sample_x : sample_x + 1), 0llu, m_width - 1);
            size_t src_y = std::clamp((delta_y < 0.5f ? sample_y : sample_y + 1), 0llu, m_height - 1);
            
            return get(src_x, src_y);
        }
    case pixmap_filter::bilinear:
        {
            size_t sample_x = static_cast<size_t>(floorf(x));
            size_t sample_y = static_cast<size_t>(floorf(y));
            float delta_x = x - sample_x;
            float delta_y = y - sample_y;
                
            size_t src_min_x = std::clamp(sample_x,     0llu, m_width - 1);
            size_t src_max_x = std::clamp(sample_x + 1, 0llu, m_width - 1);
            size_t src_min_y = std::clamp(sample_y,     0llu, m_height - 1);
            size_t src_max_y = std::clamp(sample_y + 1, 0llu, m_height - 1);

            color top_left      = get(src_min_x, src_min_y);
            color top_right     = get(src_max_x, src_min_y);
            color bottom_left   = get(src_min_x, src_max_y);
            color bottom_right  = get(src_max_x, src_max_y);

            color top_h_lerp = top_left.lerp(top_right, delta_x);
            color bottom_h_lerp = bottom_left.lerp(bottom_right, delta_x);

            color vertical_lerp = top_h_lerp.lerp(bottom_h_lerp, delta_y);

            return vertical_lerp;
        }
    }

    return color::black;
}

bool pixmap::is_channel_constant(size_t channel_index, const color& constant) const
{
    float constant_value = constant[channel_index];

    for (size_t y = 0; y < m_height; y++)
    {
        for (size_t x = 0; x < m_width; x++)
        {
            color pixel_color = get(x, y);
            if (pixel_color[channel_index] != constant_value)
            {
                return false;
            }
        }
    }

    return true;
}

bool pixmap::is_channel_one_bit(size_t channel_index) const
{
    for (size_t y = 0; y < m_height; y++)
    {
        for (size_t x = 0; x < m_width; x++)
        {
            color pixel_color = get(x, y);
            if (pixel_color[channel_index] != 0.0f &&
                pixel_color[channel_index] != 1.0f)
            {
                return false;
            }
        }
    }

    return true;
}

std::unique_ptr<pixmap> pixmap::block_encode(pixmap_format new_format, const encode_block_function_t& block_callback)
{
    pixmap_format_metrics metrics = get_pixmap_format_metrics(new_format);

    const size_t block_size = metrics.block_size;
    const size_t output_block_size = metrics.encoded_block_size;

    db_assert_message(m_format_metrics.is_mutable, "Attempting to encode a non-mutable format, this is not supported.");

    size_t total_blocks_x = std::max(1llu, m_width / block_size);
    size_t total_blocks_y = std::max(1llu, m_height / block_size);

    std::vector<uint8_t> output_data;
    output_data.resize(total_blocks_x * total_blocks_y * output_block_size, 0);

    size_t output_offset = 0;

    // Do each row in parallel. 
#if PIXMAP_PARALLEL_ENCODE
    parallel_for("encode pixmap", task_queue::loading, total_blocks_y, [this, block_size, &block_callback, &output_data, &output_offset, output_block_size, total_blocks_x, total_blocks_y](size_t y) {
#else
    for (size_t y = 0; y < total_blocks_y; y++)
    { 
#endif

        std::vector<color> pixels_rgba;
        pixels_rgba.resize((block_size * block_size), color::white);

        size_t row_output_stride = (output_block_size * total_blocks_x);

        for (size_t x = 0; x < total_blocks_x; x++)
        {
            for (size_t block_y = 0; block_y < block_size; block_y++)
            {
                for (size_t block_x = 0; block_x < block_size; block_x++)
                {
                    size_t pixel_data_offset = ((block_y * block_size) + block_x);
                    size_t pixel_x = (x * block_size) + block_x;
                    size_t pixel_y = (y * block_size) + block_y;
                    color source_color = get(pixel_x, pixel_y);
                    pixels_rgba[pixel_data_offset] = source_color;
                }
            }

            // TODO: Do we need to split into rows for output?
            size_t output_offset = (y * row_output_stride) + (x * output_block_size);
            block_callback(output_data.data() + output_offset, pixels_rgba.data());
        }
      
#if PIXMAP_PARALLEL_ENCODE
    });
#else
    }
#endif

    return std::make_unique<pixmap>(std::span{ output_data.data(), output_data.size() }, m_width, m_height, new_format);
}

std::unique_ptr<pixmap> pixmap::block_decode(pixmap_format new_format, const decode_block_function_t& block_callback)
{
    pixmap_format_metrics metrics = m_format_metrics;

    const size_t block_size = metrics.block_size;
    const size_t output_block_size = metrics.encoded_block_size;

//    db_assert_message((m_width % block_size) == 0, "Width is not multiple of compression block size.");
//    db_assert_message((m_height % block_size) == 0, "Height is not multiple of compression block size.");

    size_t total_blocks_x = std::max(1llu, m_width / block_size);
    size_t total_blocks_y = std::max(1llu, m_height / block_size);

    std::vector<color> pixels_rgba;
    pixels_rgba.resize((block_size * block_size), color::white);

    std::unique_ptr<pixmap> rgba_pixmap = std::make_unique<pixmap>(m_width, m_height, pixmap_format::R8G8B8A8);

    for (size_t block_y = 0; block_y < total_blocks_y; block_y++)
    {
        for (size_t block_x = 0; block_x < total_blocks_x; block_x++)
        {
            size_t block_offset = (block_x + block_y * total_blocks_x) * output_block_size;
            uint8_t* block_data = m_data.data() + block_offset;

            block_callback(block_data, pixels_rgba.data());

            for (size_t pixel_y = 0; pixel_y < block_size; pixel_y++)
            {
                for (size_t pixel_x = 0; pixel_x < block_size; pixel_x++)
                {
                    size_t abs_pixel_x = (block_x * block_size) + pixel_x;
                    size_t abs_pixel_y = (block_y * block_size) + pixel_y;

                    size_t pixel_offset = ((pixel_y * block_size) + pixel_x);

                    color result_color = pixels_rgba[pixel_offset];

                    // If we have a pixmap below a single block size we will have dead pixels we don't need to use.
                    if (abs_pixel_x < m_width && abs_pixel_y < m_height)
                    {
                        rgba_pixmap->set(abs_pixel_x, abs_pixel_y, result_color);
                    }
                }
            }
        }
    }

    return rgba_pixmap;
}

std::unique_ptr<pixmap> pixmap::encode_bc7(pixmap_format new_format, bool high_quality)
{
    bc7enc_compress_block_params pack_params;
    bc7enc_compress_block_params_init(&pack_params);

    if (high_quality)
    {
        pack_params.m_max_partitions_mode = BC7ENC_MAX_PARTITIONS1;
        pack_params.m_uber_level = BC7ENC_MAX_UBER_LEVEL;
    }
    else
    {
        pack_params.m_max_partitions_mode = 0;
        pack_params.m_uber_level = 0;
    }

    return block_encode(new_format, [&pack_params](uint8_t* output, color* pixels_rgba) {

        const size_t k_block_size = 4 * 4;
        uint8_t pixels[k_block_size * 4];
        for (size_t i = 0; i < k_block_size; i++)
        {
            pixels_rgba[i].get(pixels[(i * 4) + 0], pixels[(i * 4) + 1], pixels[(i * 4) + 2], pixels[(i * 4) + 3]);
        }

        bc7enc_compress_block(output, pixels, &pack_params);

    });
}

std::unique_ptr<pixmap> pixmap::decode_bc7(pixmap_format new_format)
{
    return block_decode(new_format, [](uint8_t* input, color* pixels_rgba) {

        const size_t k_block_size = 4 * 4;
        uint8_t pixels[k_block_size * 4];

        bc7decomp::unpack_bc7(input, (bc7decomp::color_rgba*)pixels_rgba);

        for (size_t i = 0; i < k_block_size; i++)
        {
            pixels_rgba[i] = color((int)pixels[(i * 4) + 0], (int)pixels[(i * 4) + 1], (int)pixels[(i * 4) + 2], (int)pixels[(i * 4) + 3]);
        }

    });
}

std::unique_ptr<pixmap> pixmap::encode_bc5(pixmap_format new_format)
{
    return block_encode(new_format, [](uint8_t* output, color* pixels_rgba) {

        const size_t k_block_size = 4 * 4;
        uint8_t pixels[k_block_size * 4];
        for (size_t i = 0; i < k_block_size; i++)
        {
            pixels_rgba[i].get(pixels[(i * 4) + 0], pixels[(i * 4) + 1], pixels[(i * 4) + 2], pixels[(i * 4) + 3]);
        }

        rgbcx::encode_bc5(output, pixels, 0, 1, 4);

    });
}

std::unique_ptr<pixmap> pixmap::decode_bc5(pixmap_format new_format)
{
    return block_decode(new_format, [](uint8_t* input, color* pixels_rgba) {

        const size_t k_block_size = 4 * 4;
        uint8_t pixels[k_block_size * 4];
        
        rgbcx::unpack_bc5(input, pixels, 0, 1, 4);

        for (size_t i = 0; i < k_block_size; i++)
        {
            pixels_rgba[i] = color((int)pixels[(i * 4) + 0], (int)pixels[(i * 4) + 1], (int)pixels[(i * 4) + 2], (int)pixels[(i * 4) + 3]);
        }        
    });
}

std::unique_ptr<pixmap> pixmap::encode_bc4(pixmap_format new_format)
{
    return block_encode(new_format, [](uint8_t* output, color* pixels_rgba) {

        const size_t k_block_size = 4 * 4;
        uint8_t pixels[k_block_size * 4];
        for (size_t i = 0; i < k_block_size; i++)
        {
            pixels_rgba[i].get(pixels[(i * 4) + 0], pixels[(i * 4) + 1], pixels[(i * 4) + 2], pixels[(i * 4) + 3]);
        }

        rgbcx::encode_bc4(output, pixels, 4);

    });
}

std::unique_ptr<pixmap> pixmap::decode_bc4(pixmap_format new_format)
{
    return block_decode(new_format, [](uint8_t* input, color* pixels_rgba) {

        const size_t k_block_size = 4 * 4;
        uint8_t pixels[k_block_size * 4];

        rgbcx::unpack_bc4(input, pixels, 4);

        for (size_t i = 0; i < k_block_size; i++)
        {
            pixels_rgba[i] = color((int)pixels[(i * 4) + 0], (int)pixels[(i * 4) + 1], (int)pixels[(i * 4) + 2], (int)pixels[(i * 4) + 3]);
        }

    });
}

std::unique_ptr<pixmap> pixmap::encode_bc3(pixmap_format new_format)
{
    return block_encode(new_format, [](uint8_t* output, color* pixels_rgba) {
        
        const size_t k_block_size = 4 * 4;
        uint8_t pixels[k_block_size * 4];
        for (size_t i = 0; i < k_block_size; i++)
        {
            pixels_rgba[i].get(pixels[(i * 4) + 0], pixels[(i * 4) + 1], pixels[(i * 4) + 2], pixels[(i * 4) + 3]);
        }
        
        rgbcx::encode_bc3(rgbcx::MAX_LEVEL, output, pixels);

    });
}

std::unique_ptr<pixmap> pixmap::decode_bc3(pixmap_format new_format)
{
    return block_decode(new_format, [](uint8_t* input, color* pixels_rgba) {

        const size_t k_block_size = 4 * 4;
        uint8_t pixels[k_block_size * 4];

        rgbcx::unpack_bc3(input, pixels);

        for (size_t i = 0; i < k_block_size; i++)
        {
            pixels_rgba[i] = color((int)pixels[(i * 4) + 0], (int)pixels[(i * 4) + 1], (int)pixels[(i * 4) + 2], (int)pixels[(i * 4) + 3]);
        }
    });
}

std::unique_ptr<pixmap> pixmap::encode_bc1(pixmap_format new_format)
{
    return block_encode(new_format, [](uint8_t* output, color* pixels_rgba) {

        const size_t k_block_size = 4 * 4;
        uint8_t pixels[k_block_size * 4];
        for (size_t i = 0; i < k_block_size; i++)
        {
            pixels_rgba[i].get(pixels[(i * 4) + 0], pixels[(i * 4) + 1], pixels[(i * 4) + 2], pixels[(i * 4) + 3]);
        }

        rgbcx::encode_bc1(rgbcx::MAX_LEVEL, output, pixels, true, false);

    });
}

std::unique_ptr<pixmap> pixmap::decode_bc1(pixmap_format new_format)
{
    return block_decode(new_format, [](uint8_t* input, color* pixels_rgba) {
        
        const size_t k_block_size = 4 * 4;
        uint8_t pixels[k_block_size * 4];
       
        rgbcx::unpack_bc1(input, pixels);

        for (size_t i = 0; i < k_block_size; i++)
        {
            pixels_rgba[i] = color((int)pixels[(i * 4) + 0], (int)pixels[(i * 4) + 1], (int)pixels[(i * 4) + 2], (int)pixels[(i * 4) + 3]);
        }

    });
}

std::unique_ptr<pixmap> pixmap::encode_bc6h_f16(pixmap_format new_format, bool is_signed, bool high_quality)
{
    void* options = nullptr;
    CreateOptionsBC6(&options);
    SetQualityBC6(options, high_quality ? 0.5f : 0.0f); // Using > 0.8 goes down a VERY slow path for little quality benefit.
    SetSignedBC6(options, is_signed);

    return block_encode(new_format, [&options, is_signed](uint8_t* output, color* pixels_rgba) {

        const size_t k_block_size = 4 * 4;
        uint16_t pixels[k_block_size * 3];
        for (size_t i = 0; i < k_block_size; i++)
        {
            float r, g, b, a;
            pixels_rgba[i].get(r, g, b, a);

            pixels[(i * 3) + 0] = math::to_float16(r);
            pixels[(i * 3) + 1] = math::to_float16(g);
            pixels[(i * 3) + 2] = math::to_float16(b);
        }

        CompressBlockBC6(pixels, 3, output, options);

    });

    DestroyOptionsBC6(options);
}

std::unique_ptr<pixmap> pixmap::decode_bc6h_f16(pixmap_format new_format, bool is_signed)
{
    void* options = nullptr;
    CreateOptionsBC6(&options);
    SetSignedBC6(options, is_signed);

    return block_decode(new_format, [&options, is_signed](uint8_t* input, color* pixels_rgba) {

        const size_t k_block_size = 4 * 4;
        uint16_t pixels[k_block_size * 3];

        DecompressBlockBC6(input, pixels, &options);

        for (size_t i = 0; i < k_block_size; i++)
        {
            pixels_rgba[i] = color(
                math::from_float16(pixels[(i * 3) + 0]),
                math::from_float16(pixels[(i * 3) + 1]),
                math::from_float16(pixels[(i * 3) + 2]),
                1.0f
            );
        }

    });

    DestroyOptionsBC6(options);
}

std::unique_ptr<pixmap> pixmap::convert(pixmap_format new_format, bool high_quality)
{
    std::unique_ptr<pixmap> result = nullptr;

    switch (new_format)
    {
        // Conversion to a compressed format.
        case pixmap_format::BC1:
        {
            result = encode_bc1(new_format);
            break;
        }
        case pixmap_format::BC3:
        {
            result = encode_bc3(new_format);
            break;
        }
        case pixmap_format::BC4:
        {
            result = encode_bc4(new_format);
            break;
        }
        case pixmap_format::BC5:
        {
            result = encode_bc5(new_format);
            break;
        }
        case pixmap_format::BC7:
        {
            result = encode_bc7(new_format, high_quality);
            break;
        }
        case pixmap_format::BC6H_SF16:
        {
            result = encode_bc6h_f16(new_format, true, high_quality);
            break;
        }
        case pixmap_format::BC6H_UF16:
        {
            result = encode_bc6h_f16(new_format, false, high_quality);
            break;
        }

        // Conversion to a linear uncompressed formats.
        default:
        {
            switch (m_format)
            {
                // Convert from a compressed format.
                case pixmap_format::BC1:
                {
                    result = decode_bc1(new_format);
                    break;
                }
                case pixmap_format::BC3:
                {
                    result = decode_bc3(new_format);
                    break;
                }
                case pixmap_format::BC4:
                {
                    result = decode_bc4(new_format);
                    break;
                }
                case pixmap_format::BC5:
                {
                    result = decode_bc5(new_format);
                    break;
                }
                case pixmap_format::BC7:
                {
                    result = decode_bc7(new_format);
                    break;
                }
                case pixmap_format::BC6H_SF16:
                {
                    result = decode_bc6h_f16(new_format, true);
                    break;
                }
                case pixmap_format::BC6H_UF16:
                {
                    result = decode_bc6h_f16(new_format, false);
                    break;
                }

                // Conversion from linear to linear, nice and easy.
                default:
                {
                    result = std::make_unique<pixmap>(m_width, m_height, new_format);
                    for (size_t y = 0; y < m_height; y++)
                    {
                        for (size_t x = 0; x < m_width; x++)
                        {
                            result->set(x, y, get(x, y));
                        }
                    }
                }
            }
        }
    }

    // Decoding of compressed formats may not be in our expected format, if it isn't
    // we need to convert to the linear format that is expected.
    if (new_format == result->get_format())
    {
        return result;
    }
    else
    {
        return result->convert(new_format);
    }
}

std::unique_ptr<pixmap> pixmap::resize(size_t width, size_t height, pixmap_filter filter)
{
    db_assert_message(m_format_metrics.is_mutable, "Attempting to resize a non-mutable image format, this is not supported.");

    std::unique_ptr<pixmap> new_pixmap = std::make_unique<pixmap>(width, height, m_format);

    float scale_factor_x = static_cast<float>(width) / static_cast<float>(m_width);
    float scale_factor_y = static_cast<float>(height) / static_cast<float>(m_height);

    for (size_t dst_y = 0; dst_y < height; dst_y++)
    {
        for (size_t dst_x = 0; dst_x < width; dst_x++)
        {
            color filtered_color = sample(dst_x / scale_factor_x, dst_y / scale_factor_y, filter);
            new_pixmap->set(dst_x, dst_y, filtered_color);
        }
    }

    return new_pixmap;
}

std::unique_ptr<pixmap> pixmap::box_resize(size_t width, size_t height)
{
    db_assert_message(m_format_metrics.is_mutable, "Attempting to resize a non-mutable image format, this is not supported.");
    db_assert_message((m_width % width) == 0 && (m_height % height) == 0 && width < m_width && height < m_height, "Box resize is only supported for shrink resizes to multiple of the original size (its meant for mipmapping).");

    std::unique_ptr<pixmap> new_pixmap = std::make_unique<pixmap>(width, height, m_format);

    size_t scale_x = (m_width / width);
    size_t scale_y = (m_height / height);

    for (size_t dst_y = 0; dst_y < height; dst_y++)
    {
        for (size_t dst_x = 0; dst_x < width; dst_x++)
        {
            color accumulated(0.0f, 0.0f, 0.0f, 0.0f);
            float sample_count = 0.0f;

            for (size_t src_y = 0; src_y < scale_y; src_y++)
            {
                for (size_t src_x = 0; src_x < scale_x; src_x++)
                {
                    color sample = get((dst_x * scale_x) + src_x, (dst_y * scale_y) + src_y);
                    accumulated.r += sample.r;
                    accumulated.g += sample.g;
                    accumulated.b += sample.b;
                    accumulated.a += sample.a;
                    sample_count += 1.0f;
                }
            }

            accumulated.r /= sample_count;
            accumulated.g /= sample_count;
            accumulated.b /= sample_count;
            accumulated.a /= sample_count;

            new_pixmap->set(dst_x, dst_y, accumulated);
        }
    }

    return new_pixmap;
}

std::unique_ptr<pixmap> pixmap::swizzle(const std::array<size_t, 4>& pattern)
{
    std::unique_ptr<pixmap> new_pixmap = std::make_unique<pixmap>(m_width, m_height, m_format);

    for (size_t y = 0; y < m_height; y++)
    {
        for (size_t x = 0; x < m_width; x++)
        {
            color src = get(x, y);
            color dst;

            for (size_t c = 0; c < 4; c++)
            {
                dst[c] = src[pattern[c]];
            }

            new_pixmap->set(x, y, dst);
        }
    }

    return new_pixmap;
}

size_t pixmap::get_width() const
{
    return m_width;
}

size_t pixmap::get_height() const
{
    return m_height;
}

pixmap_format pixmap::get_format() const
{
    return m_format;
}

std::span<const uint8_t> pixmap::get_data() const
{
    return { m_data.data(), m_data.size() };
}

bool pixmap::is_mutable()
{
    return m_format_metrics.is_mutable;
}

bool pixmap::save(const char* path)
{
    std::unique_ptr<stream> stream = virtual_file_system::get().open(path, true);
    if (!stream)
    {
        db_error(asset, "Failed to open stream when saving pixmap: %s", path);
        return false;
    }

    std::vector<char> buffer;

    std::string extension = virtual_file_system::get_extension(path);
    if (_stricmp(extension.c_str(), ".png") == 0)
    {
        if (!pixmap_png_loader::save(*this, buffer))
        {
            return false;
        }
    }
    else if (_stricmp(extension.c_str(), ".dds") == 0)
    {
        if (!pixmap_dds_loader::save(*this, buffer))
        {
            return false;
        }
    }
    else if (_stricmp(extension.c_str(), ".tga") == 0 ||
             _stricmp(extension.c_str(), ".jpeg") == 0 ||
             _stricmp(extension.c_str(), ".jpg") == 0 || 
             _stricmp(extension.c_str(), ".bmp") == 0 || 
             _stricmp(extension.c_str(), ".psd") == 0 || 
             _stricmp(extension.c_str(), ".gif") == 0 || 
             _stricmp(extension.c_str(), ".hdr") == 0 || 
             _stricmp(extension.c_str(), ".pic") == 0 || 
             _stricmp(extension.c_str(), ".pnm") == 0)
    {
        if (!pixmap_stb_loader::save(*this, buffer))
        {
            return false;
        }
    }
    else
    {
        db_error(asset, "Failed to determine file format when saving pixmap: %s", path);
        return false;
    }

    if (stream->write(buffer.data(), buffer.size()) != buffer.size())
    {
        db_error(asset, "Failed to write full file when saving pixmap: %s", path);
        return false;
    }

    return true;
}

std::vector<std::unique_ptr<pixmap>> pixmap::load(const char* path)
{
    std::unique_ptr<stream> stream = virtual_file_system::get().open(path, false);
    if (!stream)
    {
        db_error(asset, "Failed to open stream when loading pixmap: %s", path);
        return {};
    }

    std::vector<char> buffer;
    buffer.resize(stream->length());
    if (stream->read(buffer.data(), buffer.size()) != buffer.size())
    {
        db_error(asset, "Failed to read full file when loading pixmap: %s", path);
        return {};
    }

    std::vector<std::unique_ptr<pixmap>> result;

    std::string extension = virtual_file_system::get_extension(path);

    // We can use STB for png loading, but its considerably slower.
    if (_stricmp(extension.c_str(), ".png") == 0)
    {
        result = pixmap_png_loader::load(buffer);
    }
    else if (_stricmp(extension.c_str(), ".dds") == 0)
    {
        result = pixmap_dds_loader::load(buffer);
    }
    else if (_stricmp(extension.c_str(), ".tga") == 0 ||
             _stricmp(extension.c_str(), ".jpeg") == 0 ||
             _stricmp(extension.c_str(), ".jpg") == 0 || 
             _stricmp(extension.c_str(), ".bmp") == 0 || 
             _stricmp(extension.c_str(), ".psd") == 0 || 
             _stricmp(extension.c_str(), ".gif") == 0 || 
             _stricmp(extension.c_str(), ".hdr") == 0 || 
             _stricmp(extension.c_str(), ".pic") == 0 || 
             _stricmp(extension.c_str(), ".pnm") == 0)
    {
        result = pixmap_stb_loader::load(buffer);
    }
    else
    {
        db_error(asset, "Failed to determine file format when loading pixmap: %s", path);
        return {};
    }

    return result;
}

}; // namespace workshop
