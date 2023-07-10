// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/containers/string.h"
#include "workshop.core/math/math.h"
#include "workshop.core/math/vector4.h"

#include <string>
#include <array>
#include <vector>

namespace ws {

// ================================================================================================
//  Represents an arbitrary color as 4 float channels; red, green, blue, alpha
// ================================================================================================
class color
{
public:
    float r, g, b, a;
    
    color()
        : r(0.0f)
        , g(0.0f)
        , b(0.0f)
        , a(1.0f)
    {
    }

    color(int in_r, int in_g, int in_b, int in_a)
        : r(in_r / 255.0f)
        , g(in_g / 255.0f)
        , b(in_b / 255.0f)
        , a(in_a / 255.0f)
    {
    }

    color(float in_r, float in_g, float in_b, float in_a)
        : r(in_r)
        , g(in_g)
        , b(in_b)
        , a(in_a)
    {
    }
    
    float& operator[](size_t idx)
    {
        if (idx == 0)
        {
            return r;
        }
        else if (idx == 1)
        {
            return g;
        }
        else if (idx == 2)
        {
            return b;
        }
        else if (idx == 3)
        {
            return a;
        }
        throw new std::out_of_range("Color channel index out of range.");
    }

    const float& operator[](std::size_t idx) const 
    {
        if (idx == 0)
        {
            return r;
        }
        else if (idx == 1)
        {
            return g;
        }
        else if (idx == 2)
        {
            return b;
        }
        else if (idx == 3)
        {
            return a;
        }
        throw new std::out_of_range("Color channel index out of range.");
    }

    inline void get(uint8_t& out_r, uint8_t& out_g, uint8_t& out_b, uint8_t& out_a) const
    {
        out_r = static_cast<uint8_t>(255 * r);
        out_g = static_cast<uint8_t>(255 * g);
        out_b = static_cast<uint8_t>(255 * b);
        out_a = static_cast<uint8_t>(255 * a);
    }

    inline vector4 argb() const
    {
        return vector4(r, g, b, a);
    }

    inline color lerp(const color& to, float alpha) const
    {
        return {
            math::lerp(r, to.r, alpha),
            math::lerp(g, to.g, alpha),
            math::lerp(b, to.b, alpha),
            math::lerp(a, to.a, alpha)
        };
    }

public:
    const static color black;
    const static color white;
    const static color pure_red;
    const static color pure_green;
    const static color pure_blue;
    const static color red;
    const static color pink;
    const static color purple;
    const static color deep_purple;
    const static color indigo;
    const static color blue;
    const static color light_blue;
    const static color cyan;
    const static color teal;
    const static color green;
    const static color light_green;
    const static color lime;
    const static color yellow;
    const static color amber;
    const static color orange;
    const static color deep_orange;
    const static color brown;
    const static color grey;
    const static color blue_grey;

};

inline const color color::black         = { 0.0f, 0.0f, 0.0f, 1.0f };
inline const color color::white         = { 1.0f, 1.0f, 1.0f, 1.0f };
inline const color color::pure_red      = { 1.0f, 0.0f, 0.0f, 1.0f };
inline const color color::pure_green    = { 0.0f, 1.0f, 0.0f, 1.0f };
inline const color color::pure_blue     = { 0.0f, 0.0f, 1.0f, 1.0f };
inline const color color::red           = { 229, 115, 115, 255 };
inline const color color::pink          = { 240, 98, 146, 255 };
inline const color color::purple        = { 186, 104, 200, 255 };
inline const color color::deep_purple   = { 149, 117, 205, 255 };
inline const color color::indigo        = { 121, 134, 203, 255 };
inline const color color::blue          = { 100, 181, 246, 255 };
inline const color color::light_blue    = { 79, 195, 247, 255 };
inline const color color::cyan          = { 77, 208, 225, 255 };
inline const color color::teal          = { 77, 182, 172, 255 };
inline const color color::green         = { 129, 199, 132, 255 };
inline const color color::light_green   = { 174, 213, 129, 255 };
inline const color color::lime          = { 220, 231, 117, 255 };
inline const color color::yellow        = { 255, 241, 118, 255 };
inline const color color::amber         = { 255, 213, 79, 255 };
inline const color color::orange        = { 255, 183, 77, 255 };
inline const color color::deep_orange   = { 255, 138, 101, 255 };
inline const color color::brown         = { 161, 136, 127, 255 };
inline const color color::grey          = { 224, 224, 224, 255 };
inline const color color::blue_grey     = { 144, 164, 174, 255 };

}; // namespace workshop
