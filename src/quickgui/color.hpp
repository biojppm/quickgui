#ifndef QUICKGUI_COLOR_HPP_
#define QUICKGUI_COLOR_HPP_

#include <cstdint>
#include <quickgui/mem.hpp>
#include <quickgui/math.hpp>

namespace quickgui {

struct ucolor;
struct fcolor;


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct fcolor
{
    using value_type = float;
    float r, g, b, a;
    fcolor() noexcept = default;
    explicit fcolor(uint32_t c) noexcept : r(), g(), b(), a() { set(c); }
    constexpr explicit fcolor(float r_, float g_, float b_) noexcept : r(r_), g(g_), b(b_), a(1.f) {}
    constexpr explicit fcolor(float r_, float g_, float b_, float a_) noexcept : r(r_), g(g_), b(b_), a(a_) {}
    constexpr explicit fcolor(uint8_t r_, uint8_t g_, uint8_t b_) noexcept : r(r_ / 255.f), g(g_ / 255.f), b(b_ / 255.f), a(1.f) {}
    constexpr explicit fcolor(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_) noexcept : r(r_ / 255.f), g(g_ / 255.f), b(b_ / 255.f), a(a_ / 255.f) {}
    explicit fcolor(ucolor c) noexcept;
    fcolor& operator=(uint32_t c) noexcept { set(c); return *this; }
    operator ucolor() const noexcept;
    operator uint32_t() const noexcept
    {
        return uint32_t(r * 255.f)
            | (uint32_t(g * 255.f) << 8)
            | (uint32_t(b * 255.f) << 16)
            | (uint32_t(a * 255.f) << 24);
    }
    void set(uint32_t c) noexcept
    {
        r = float((c & UINT32_C(0x00'00'00'ff))      ) / 255.f;
        g = float((c & UINT32_C(0x00'00'ff'00)) >>  8) / 255.f;
        b = float((c & UINT32_C(0x00'ff'00'00)) >> 16) / 255.f;
        a = float((c & UINT32_C(0xff'00'00'00)) >> 24) / 255.f;
    }
    void clamp() noexcept
    {
        r = clamp01(r);
        g = clamp01(g);
        b = clamp01(b);
        a = clamp01(a);
    }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct ucolor
{
    using value_type = uint8_t;
    uint8_t r, g, b, a;
    operator fcolor() const noexcept;
    explicit ucolor(fcolor c) noexcept;
    explicit ucolor() noexcept : r(), g(), b(), a() {};
    constexpr explicit ucolor(uint8_t r_, uint8_t g_, uint8_t b_) noexcept : r(r_), g(g_), b(b_), a(UINT8_C(0xff)) {}
    constexpr explicit ucolor(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_) noexcept : r(r_), g(g_), b(b_), a(a_) {}
    ucolor& operator=(uint32_t c) noexcept { set(c); return *this; }
    explicit ucolor(uint32_t c) noexcept
        : r(uint8_t((c & UINT32_C(0x00'00'00'ff))      ))
        , g(uint8_t((c & UINT32_C(0x00'00'ff'00)) >>  8))
        , b(uint8_t((c & UINT32_C(0x00'ff'00'00)) >> 16))
        , a(uint8_t((c & UINT32_C(0xff'00'00'00)) >> 24))
    {}
    void set(uint32_t c) noexcept
    {
        r = uint8_t((c & UINT32_C(0x00'00'00'ff))      );
        g = uint8_t((c & UINT32_C(0x00'00'ff'00)) >>  8);
        b = uint8_t((c & UINT32_C(0x00'ff'00'00)) >> 16);
        a = uint8_t((c & UINT32_C(0xff'00'00'00)) >> 24);
    }
    operator uint32_t() const noexcept
    {
        return uint32_t(r)
            | (uint32_t(g) << 8)
            | (uint32_t(b) << 16)
            | (uint32_t(a) << 24);
    }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

C4_CONST C4_ALWAYS_INLINE fcolor tof(ucolor c) noexcept
{
    return fcolor(c.r, c.g, c.b, c.a);
}
C4_CONST C4_ALWAYS_INLINE ucolor tou(fcolor c) noexcept
{
    return ucolor(
        uint8_t(c.r * 255.f),
        uint8_t(c.g * 255.f),
        uint8_t(c.b * 255.f),
        uint8_t(c.a * 255.f));
}


inline fcolor::fcolor(ucolor c) noexcept : fcolor(tof(c))
{
}
inline ucolor::ucolor(fcolor c) noexcept : ucolor(tou(c))
{
}


inline fcolor::operator ucolor() const noexcept
{
    return tou(*this);
}
inline ucolor::operator fcolor() const noexcept
{
    return tof(*this);
}


C4_CONST C4_ALWAYS_INLINE ucolor alpha(ucolor c, uint8_t alpha)
{
    c.a = alpha;
    return c;
}
C4_CONST C4_ALWAYS_INLINE ucolor alpha(fcolor c, float alpha)
{
    c.a = alpha;
    return c;
}

C4_CONST C4_ALWAYS_INLINE fcolor clamp(fcolor ret)
{
    ret.r = ret.r > 0.f ? ret.r : 0.f;
    ret.g = ret.g > 0.f ? ret.g : 0.f;
    ret.b = ret.b > 0.f ? ret.b : 0.f;
    ret.r = ret.r < 1.f ? ret.r : 1.f;
    ret.g = ret.g < 1.f ? ret.g : 1.f;
    ret.b = ret.b < 1.f ? ret.b : 1.f;
    return ret;
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct ucolors
{
    constexpr inline static const ucolor red  {UINT8_C(0xff), UINT8_C(0x0 ), UINT8_C(0x0 )};
    constexpr inline static const ucolor green{UINT8_C(0x0 ), UINT8_C(0xff), UINT8_C(0x0 )};
    constexpr inline static const ucolor blue {UINT8_C(0x0 ), UINT8_C(0x0 ), UINT8_C(0xff)};
    constexpr inline static const ucolor yellow{UINT8_C(0xff), UINT8_C(0xff), UINT8_C(0x0 )};
    constexpr inline static const ucolor black{UINT8_C(0x0 ), UINT8_C(0x0 ), UINT8_C(0x0 )};
    constexpr inline static const ucolor gray1{UINT8_C(0x11), UINT8_C(0x11), UINT8_C(0x11)};
    constexpr inline static const ucolor gray2{UINT8_C(0x22), UINT8_C(0x22), UINT8_C(0x22)};
    constexpr inline static const ucolor gray3{UINT8_C(0x33), UINT8_C(0x33), UINT8_C(0x33)};
    constexpr inline static const ucolor gray4{UINT8_C(0x44), UINT8_C(0x44), UINT8_C(0x44)};
    constexpr inline static const ucolor gray5{UINT8_C(0x55), UINT8_C(0x55), UINT8_C(0x55)};
    constexpr inline static const ucolor gray6{UINT8_C(0x66), UINT8_C(0x66), UINT8_C(0x66)};
    constexpr inline static const ucolor gray7{UINT8_C(0x77), UINT8_C(0x77), UINT8_C(0x77)};
    constexpr inline static const ucolor gray8{UINT8_C(0x88), UINT8_C(0x88), UINT8_C(0x88)};
    constexpr inline static const ucolor gray9{UINT8_C(0x99), UINT8_C(0x99), UINT8_C(0x99)};
    constexpr inline static const ucolor graya{UINT8_C(0xaa), UINT8_C(0xaa), UINT8_C(0xaa)};
    constexpr inline static const ucolor grayb{UINT8_C(0xbb), UINT8_C(0xbb), UINT8_C(0xbb)};
    constexpr inline static const ucolor grayc{UINT8_C(0xcc), UINT8_C(0xcc), UINT8_C(0xcc)};
    constexpr inline static const ucolor grayd{UINT8_C(0xdd), UINT8_C(0xdd), UINT8_C(0xdd)};
    constexpr inline static const ucolor graye{UINT8_C(0xee), UINT8_C(0xee), UINT8_C(0xee)};
    constexpr inline static const ucolor white{UINT8_C(0xff), UINT8_C(0xff), UINT8_C(0xff)};
    constexpr inline static const ucolor reddish{UINT8_C(0xff), UINT8_C(0x7f), UINT8_C(0x7f)};
    constexpr inline static const ucolor greenish{UINT8_C(0x7f), UINT8_C(0xff), UINT8_C(0x7f)};
    constexpr inline static const ucolor blueish{UINT8_C(0x7f), UINT8_C(0x7f), UINT8_C(0xff)};
    constexpr inline static const ucolor yellowish{UINT8_C(0xff), UINT8_C(0xff), UINT8_C(0x7f)};
};

} // namespace quickgui

#endif /* QUICKGUI_COLOR_HPP_ */
