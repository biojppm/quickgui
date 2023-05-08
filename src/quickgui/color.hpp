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
    constexpr explicit ucolor(uint32_t c) noexcept
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
C4_CONST C4_ALWAYS_INLINE ucolor alpha(ucolor c, float alpha)
{
    c.a = (uint8_t)(clamp01(alpha) * 255.f);
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

/** for palettes @see palettes.hpp */
struct colors
{
#define _defcolor4(name, r,g,b,a) constexpr inline static const ucolor name{UINT8_C(0x##r), UINT8_C(0x##g), UINT8_C(0x##b), UINT8_C(0x##a)}
#define _defcolor(name, r,g,b) constexpr inline static const ucolor name{UINT8_C(0x##r), UINT8_C(0x##g), UINT8_C(0x##b)}
    _defcolor4(none,     00,00,00,00);
    _defcolor(blue,      1f,77,b4);
    _defcolor(orange,    ff,7f,0e);
    _defcolor(green,     2c,a0,2c);
    _defcolor(red,       d6,27,28);
    _defcolor(purple,    94,67,bd);
    _defcolor(brown,     8c,56,4b);
    _defcolor(pink,      e3,77,c2);
    _defcolor(gray,      7f,7f,7f);
    _defcolor(olive,     bc,bd,22);
    _defcolor(cyan,      17,be,cf);

    _defcolor(red_pure,  ff,00,00);
    _defcolor(green_pure,00,ff,00);
    _defcolor(blue_pure, 00,00,f0);
    _defcolor(yellow_pure,ff,ff,00);
    _defcolor(cyan_pure, 00,ff,ff);
    _defcolor(black,     00,00,00);
    _defcolor(gray1,     11,11,11);
    _defcolor(gray2,     22,22,22);
    _defcolor(gray3,     33,33,33);
    _defcolor(gray4,     44,44,44);
    _defcolor(gray5,     55,55,55);
    _defcolor(gray6,     66,66,66);
    _defcolor(gray7,     77,77,77);
    _defcolor(gray8,     88,88,88);
    _defcolor(gray9,     99,99,99);
    _defcolor(graya,     aa,aa,aa);
    _defcolor(grayb,     bb,bb,bb);
    _defcolor(grayc,     cc,cc,cc);
    _defcolor(grayd,     dd,dd,dd);
    _defcolor(graye,     ee,ee,ee);
    _defcolor(white,     ff,ff,ff);
    _defcolor(reddish,   ff,7f,70);
    _defcolor(greenish,  7f,ff,70);
    _defcolor(blueish,   7f,7f,f0);
    _defcolor(yellowish, ff,ff,70);

    _defcolor(aliceblue,    F0,F8,FF);
    _defcolor(antiquewhite, FA,EB,D7);
    _defcolor(aqua,         00,FF,FF);
    _defcolor(aquamarine, 7F,FF,D4);
    _defcolor(azure, F0,FF,FF);
    _defcolor(beige, F5,F5,DC);
    _defcolor(bisque, FF,E4,C4);
    _defcolor(blanchedalmond, FF,EB,CD);
    _defcolor(blueviolet, 8A,2B,E2);
    _defcolor(brown2, A5,2A,2A);
    _defcolor(burlywood, DE,B8,87);
    _defcolor(cadetblue, 5F,9E,A0);
    _defcolor(chartreuse, 7F,FF,00);
    _defcolor(chocolate, D2,69,1E);
    _defcolor(coral, FF,7F,50);
    _defcolor(cornflowerblue, 64,95,ED);
    _defcolor(cornsilk, FF,F8,DC);
    _defcolor(crimson, DC,14,3C);
    _defcolor(darkblue, 00,00,8B);
    _defcolor(darkcyan, 00,8B,8B);
    _defcolor(darkgoldenrod, B8,86,0B);
    _defcolor(darkgray, A9,A9,A9);
    _defcolor(darkgreen, 00,64,00);
    _defcolor(darkgrey, A9,A9,A9);
    _defcolor(darkkhaki, BD,B7,6B);
    _defcolor(darkmagenta, 8B,00,8B);
    _defcolor(darkolivegreen, 55,6B,2F);
    _defcolor(darkorange, FF,8C,00);
    _defcolor(darkorchid, 99,32,CC);
    _defcolor(darkred, 8B,00,00);
    _defcolor(darksalmon, E9,96,7A);
    _defcolor(darkseagreen, 8F,BC,8F);
    _defcolor(darkslateblue, 48,3D,8B);
    _defcolor(darkslategray, 2F,4F,4F);
    _defcolor(darkslategrey, 2F,4F,4F);
    _defcolor(darkturquoise, 00,CE,D1);
    _defcolor(darkviolet, 94,00,D3);
    _defcolor(deeppink, FF,14,93);
    _defcolor(deepskyblue, 00,BF,FF);
    _defcolor(dimgray, 69,69,69);
    _defcolor(dimgrey, 69,69,69);
    _defcolor(dodgerblue, 1E,90,FF);
    _defcolor(firebrick, B2,22,22);
    _defcolor(floralwhite, FF,FA,F0);
    _defcolor(forestgreen, 22,8B,22);
    _defcolor(fuchsia, FF,00,FF);
    _defcolor(gainsboro, DC,DC,DC);
    _defcolor(ghostwhite, F8,F8,FF);
    _defcolor(gold, FF,D7,00);
    _defcolor(goldenrod, DA,A5,20);
    _defcolor(green2, 00,80,00);
    _defcolor(greenyellow, AD,FF,2F);
    _defcolor(grey, 80,80,80);
    _defcolor(honeydew, F0,FF,F0);
    _defcolor(hotpink, FF,69,B4);
    _defcolor(indianred, CD,5C,5C);
    _defcolor(indigo, 4B,00,82);
    _defcolor(ivory, FF,FF,F0);
    _defcolor(khaki, F0,E6,8C);
    _defcolor(lavender, E6,E6,FA);
    _defcolor(lavenderblush, FF,F0,F5);
    _defcolor(lawngreen, 7C,FC,00);
    _defcolor(lemonchiffon, FF,FA,CD);
    _defcolor(lightblue, AD,D8,E6);
    _defcolor(lightcoral, F0,80,80);
    _defcolor(lightcyan, E0,FF,FF);
    _defcolor(lightgoldenrodyellow, FA,FA,D2);
    _defcolor(lightgray, D3,D3,D3);
    _defcolor(lightgreen, 90,EE,90);
    _defcolor(lightgrey, D3,D3,D3);
    _defcolor(lightpink, FF,B6,C1);
    _defcolor(lightsalmon, FF,A0,7A);
    _defcolor(lightseagreen, 20,B2,AA);
    _defcolor(lightskyblue, 87,CE,FA);
    _defcolor(lightslategray, 77,88,99);
    _defcolor(lightslategrey, 77,88,99);
    _defcolor(lightsteelblue, B0,C4,DE);
    _defcolor(lightyellow, FF,FF,E0);
    _defcolor(lime, 00,FF,00);
    _defcolor(limegreen, 32,CD,32);
    _defcolor(linen, FA,F0,E6);
    _defcolor(magenta, FF,00,FF);
    _defcolor(maroon, 80,00,00);
    _defcolor(mediumaquamarine, 66,CD,AA);
    _defcolor(mediumblue, 00,00,CD);
    _defcolor(mediumorchid, BA,55,D3);
    _defcolor(mediumpurple, 93,70,DB);
    _defcolor(mediumseagreen, 3C,B3,71);
    _defcolor(mediumslateblue, 7B,68,EE);
    _defcolor(mediumspringgreen, 00,FA,9A);
    _defcolor(mediumturquoise, 48,D1,CC);
    _defcolor(mediumvioletred, C7,15,85);
    _defcolor(midnightblue, 19,19,70);
    _defcolor(mintcream, F5,FF,FA);
    _defcolor(mistyrose, FF,E4,E1);
    _defcolor(moccasin, FF,E4,B5);
    _defcolor(navajowhite, FF,DE,AD);
    _defcolor(navy, 00,00,80);
    _defcolor(oldlace, FD,F5,E6);
    _defcolor(olive2, 80,80,00);
    _defcolor(olivedrab, 6B,8E,23);
    _defcolor(orange2, FF,A5,00);
    _defcolor(orangered, FF,45,00);
    _defcolor(orchid, DA,70,D6);
    _defcolor(palegoldenrod, EE,E8,AA);
    _defcolor(palegreen, 98,FB,98);
    _defcolor(paleturquoise, AF,EE,EE);
    _defcolor(palevioletred, DB,70,93);
    _defcolor(papayawhip, FF,EF,D5);
    _defcolor(peachpuff, FF,DA,B9);
    _defcolor(peru, CD,85,3F);
    _defcolor(pink2, FF,C0,CB);
    _defcolor(plum, DD,A0,DD);
    _defcolor(powderblue, B0,E0,E6);
    _defcolor(purple2, 80,00,80);
    _defcolor(rebeccapurple, 66,33,99);
    _defcolor(red2, FF,00,00);
    _defcolor(rosybrown, BC,8F,8F);
    _defcolor(royalblue, 41,69,E1);
    _defcolor(saddlebrown, 8B,45,13);
    _defcolor(salmon, FA,80,72);
    _defcolor(sandybrown, F4,A4,60);
    _defcolor(seagreen, 2E,8B,57);
    _defcolor(seashell, FF,F5,EE);
    _defcolor(sienna, A0,52,2D);
    _defcolor(silver, C0,C0,C0);
    _defcolor(skyblue, 87,CE,EB);
    _defcolor(slateblue, 6A,5A,CD);
    _defcolor(slategray, 70,80,90);
    _defcolor(slategrey, 70,80,90);
    _defcolor(snow, FF,FA,FA);
    _defcolor(springgreen, 00,FF,7F);
    _defcolor(steelblue, 46,82,B4);
    _defcolor(tan, D2,B4,8C);
    _defcolor(teal, 00,80,80);
    _defcolor(thistle, D8,BF,D8);
    _defcolor(tomato, FF,63,47);
    _defcolor(turquoise, 40,E0,D0);
    _defcolor(violet, EE,82,EE);
    _defcolor(wheat, F5,DE,B3);
    _defcolor(whitesmoke, F5,F5,F5);
    _defcolor(yellowgreen, 9A,CD,32);
#undef _defcolor
#undef _defcolor4
};

} // namespace quickgui

#endif /* QUICKGUI_COLOR_HPP_ */
