// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/drawing/color.h"

#include <span>
#include <memory>
#include <vector>
#include <functional>

namespace ws {

// ================================================================================================
//  Formats that a pixmap can be internally stored in.
// ================================================================================================
enum class pixmap_format
{
    // These formats can be directly manipulated.
    R32G32B32A32_FLOAT,
    R32G32B32A32_SIGNED,
    R32G32B32A32,

    R32G32B32_FLOAT,
    R32G32B32_SIGNED,
    R32G32B32,

    R32G32_FLOAT,
    R32G32_SIGNED,
    R32G32,

    R32_FLOAT,
    R32_SIGNED,
    R32,

    R16G16B16A16_FLOAT,
    R16G16B16A16_SIGNED,
    R16G16B16A16,

    R16G16_FLOAT,
    R16G16_SIGNED,
    R16G16,

    R16_FLOAT,
    R16_SIGNED,
    R16,

    R8G8B8A8_SIGNED,
    R8G8B8A8,

    R8G8,
    R8G8_SIGNED,

    R8,
    R8_SIGNED,

    // These formats cannot be manipulated, the pixmap is expected to be just a 
    // constant storage container once the format is converted to one of these.
    BC1,
    //BC2, // We don't support this, as there is no reason to ever use it over BC3.
    BC3,
    BC4,
    BC5,
    //BC6, // TODO: When we need to support HDR textures.
    BC7,

    COUNT
};

inline static const char* pixmap_format_strings[static_cast<int>(pixmap_format::COUNT)] = {
    "R32G32B32A32_FLOAT",
    "R32G32B32A32_SIGNED",
    "R32G32B32A32",
    
    "R32G32B32_FLOAT",
    "R32G32B32_SIGNED",
    "R32G32B32",
    
    "R32G32_FLOAT",
    "R32G32_SIGNED",
    "R32G32",
    
    "R32_FLOAT",
    "R32_SIGNED",
    "R32",
    
    "R16G16B16A16_FLOAT",
    "R16G16B16A16_SIGNED",
    "R16G16B16A16",
    
    "R16G16_FLOAT",
    "R16G16_SIGNED",
    "R16G16",
    
    "R16_FLOAT",
    "R16_SIGNED",
    "R16",
    
    "R8G8B8A8_SIGNED",
    "R8G8B8A8",
    
    "R8G8",
    "R8G8_SIGNED",
    
    "R8",
    "R8_SIGNED",
    
    "BC1",
    "BC3",
    "BC4",
    "BC5",
    "BC7",
};

DEFINE_ENUM_TO_STRING(pixmap_format, pixmap_format_strings)

// Data type that a channel is stored in.
enum class pixmap_channel_format
{
    t_unsigned_int,
    t_signed_int,
    t_float
};

// Holds some general metrics on the pixmap_format values. 
struct pixmap_format_metrics
{
    size_t pixel_size;
    std::vector<size_t> channels; // 
    size_t channel_size;
    pixmap_channel_format channel_format;
    bool is_mutable;
    size_t block_size;
    size_t encoded_block_size;
};

// Filters that can be used when resizing/sampling a pixmap.
enum class pixmap_filter
{
    nearest_neighbour,
    bilinear,
};

// Gets the metrics for a given pixmap format.
pixmap_format_metrics get_pixmap_format_metrics(pixmap_format format);

// ================================================================================================
//  Represents a 2 dimensional grid of pixels that can be manipulated.
// 
//  Because of the potential format handling that needs to be handled, reading or writing pixmaps
//  can be slow. Faster manipulation is possible if you modify in one of the following formats
//  which have fast-paths available for them. If you need another format, consider modifying in 
//  these formats before converting to your target format when finished.
// 
//  R32G32B32A32_FLOAT
//  R8G8B8A8
// 
//  Compressed formats cannot be directly read or written from, the pixmap just acts are storage
//  for them. If you want to manipulate them convert them to a linear format and then convert back
//  when you are finished. Re-encoding compressed formats is expensive, so avoid doing so where 
//  possible.
// 
//  Be aware that pixmap manipulation is mainly meant for creating cooked assets.
//  Don't rely on it being fast enough for realtime code.
// ================================================================================================
class pixmap
{
public:

    // Creates a pixmap from the given data buffer assuming linear layout of row major 
    // with the given format.
    pixmap(std::span<uint8_t> data, size_t width, size_t height, pixmap_format format);

    // Creates a pixmap of the given specs and zeros it.
    pixmap(size_t width, size_t height, pixmap_format format);

    // Sets the color of the given pixel.
    void set(size_t x, size_t y, const color& color);

    // Gets the color of the given pixel.
    color get(size_t x, size_t y) const;

    // Gets the color of the given pixel, if value is between multiple pixels the value will be interpolated.
    color sample(float x, float y, pixmap_filter filter);

    // Gets the width of the pixmap.
    size_t get_width() const;

    // Gets the height of the pixmap.
    size_t get_height() const;

    // Gets the format of the pixmap.
    pixmap_format get_format() const;

    // Gets a span of the raw pixmap data.
    std::span<const uint8_t> get_data() const;

    // Returns true if all values for a given channel match the value of the channel in the provided color.
    bool is_channel_constant(size_t channel_index, const color& color) const;

    // Returns true if all values for a given channel are either the min or max values for it.
    bool is_channel_one_bit(size_t channel_index) const;

    // Creates a new pixmap that contains the contents of this pixmap converted
    // to a different format.
    std::unique_ptr<pixmap> convert(pixmap_format new_format);

    // Creates a new pixmap that is resized to the given size.
    std::unique_ptr<pixmap> resize(size_t width, size_t height, pixmap_filter filter);

    // Returns true if the pixmap can be modified or read from. The pixmap can only be modified if its in
    // one of the modifiable formats. For things such as compressed formats, the pixmap class
    // acts only as storage, and cannot be modified.
    bool is_mutable();

    // Attempts to save the pixmap data to the given image file.
    // Returns false if not able to save or attempting to save an unsupported format.
    bool save(const char* path);

    // Attempts to load the pixmap data from the given image file.
    // Returns nullptr if not able to load or attempting to load an unsupported format.
    static std::unique_ptr<pixmap> load(const char* path);

private:

    using encode_block_function_t = std::function<void(uint8_t* output, uint8_t* pixels_rgba)>;
    using decode_block_function_t = std::function<void(uint8_t* input, uint8_t* pixels_rgba)>;

    std::unique_ptr<pixmap> block_encode(pixmap_format new_format, const encode_block_function_t& block_callback);
    std::unique_ptr<pixmap> block_decode(pixmap_format new_format, const decode_block_function_t& block_callback);

    // Conversion to/from the various compressed formats.
    std::unique_ptr<pixmap> encode_bc7(pixmap_format new_format);
    std::unique_ptr<pixmap> decode_bc7(pixmap_format new_format);

    std::unique_ptr<pixmap> encode_bc5(pixmap_format new_format);
    std::unique_ptr<pixmap> decode_bc5(pixmap_format new_format);

    std::unique_ptr<pixmap> encode_bc4(pixmap_format new_format);
    std::unique_ptr<pixmap> decode_bc4(pixmap_format new_format);

    std::unique_ptr<pixmap> encode_bc3(pixmap_format new_format);
    std::unique_ptr<pixmap> decode_bc3(pixmap_format new_format);

    std::unique_ptr<pixmap> encode_bc1(pixmap_format new_format);
    std::unique_ptr<pixmap> decode_bc1(pixmap_format new_format);

private:

    std::vector<uint8_t> m_data;
    size_t m_width;
    size_t m_height;
    size_t m_row_stride;
    pixmap_format m_format;
    pixmap_format_metrics m_format_metrics;

};

}; // namespace workshop
