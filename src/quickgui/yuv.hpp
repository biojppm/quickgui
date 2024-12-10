#ifndef QUICKGUI_YUV_HPP_
#define QUICKGUI_YUV_HPP_

#include <quickgui/color.hpp>

namespace quickgui {

/** https://en.wikipedia.org/wiki/YUV
 * @see to_yuv_sdtv
 * @see to_rgb_sdtv
 * @see to_yuv_hdtv
 * @see to_rgb_hdtv
 * */
struct yuv
{
    yuv() noexcept = default;
    constexpr explicit yuv(float y_, float u_, float v_, float a_) noexcept : y(y_), u(u_), v(v_), a(a_) {}
    constexpr explicit yuv(float y_, float u_, float v_) noexcept : y(y_), u(u_), v(v_), a(1.f) {}
    constexpr explicit yuv(uint8_t y_, uint8_t u_, uint8_t v_, uint8_t a_) noexcept : y(((float)y_) / 255.f), u(((float)u_) / 255.f), v(((float)v_) / 255.f), a(((float)a_) / 255.f) {}
    constexpr explicit yuv(uint8_t y_, uint8_t u_, uint8_t v_) noexcept : y(((float)y_) / 255.f), u(((float)u_) / 255.f), v(((float)v_) / 255.f), a(1.f) {}
    float y, u, v, a;
};
struct yuv3
{
    yuv3() noexcept = default;
    constexpr explicit yuv3(float y_, float u_, float v_) noexcept : y(y_), u(u_), v(v_) {}
    constexpr explicit yuv3(uint8_t y_, uint8_t u_, uint8_t v_) noexcept : y(((float)y_) / 255.f), u(((float)u_) / 255.f), v(((float)v_) / 255.f) {}
    float y, u, v;
};

/** https://en.wikipedia.org/wiki/YUV */
C4_CONST C4_ALWAYS_INLINE yuv to_yuv_sdtv(fcolor c) noexcept
{
    yuv ret;
    ret.y =  0.299f   * c.r +  0.58700f * c.g +  0.11400f * c.b;
    ret.u = -0.14713f * c.r + -0.28886f * c.g +  0.43600f * c.b;
    ret.v =  0.615f   * c.r + -0.51499f * c.g + -0.10001f * c.b;
    ret.a = c.a;
    return ret;
}
/** https://en.wikipedia.org/wiki/YUV */
C4_CONST C4_ALWAYS_INLINE yuv3 to_yuv_sdtv(fcolor3 c) noexcept
{
    yuv3 ret;
    ret.y =  0.299f   * c.r +  0.58700f * c.g +  0.11400f * c.b;
    ret.u = -0.14713f * c.r + -0.28886f * c.g +  0.43600f * c.b;
    ret.v =  0.615f   * c.r + -0.51499f * c.g + -0.10001f * c.b;
    return ret;
}
C4_CONST C4_ALWAYS_INLINE float to_luminance_sdtv(fcolor c) noexcept
{
    return 0.299f * c.r + 0.58700f * c.g + 0.11400f * c.b;
}
C4_CONST C4_ALWAYS_INLINE float to_luminance_sdtv(fcolor3 c) noexcept
{
    return 0.299f * c.r + 0.58700f * c.g + 0.11400f * c.b;
}

/** https://en.wikipedia.org/wiki/YUV */
C4_CONST C4_ALWAYS_INLINE fcolor to_rgb_sdtv(yuv c) noexcept
{
    fcolor ret;
    ret.r = c.y + /*    0.f * c.u+*/ 1.13983f * c.v;
    ret.g = c.y + -0.39465f * c.u + -0.58060f * c.v;
    ret.b = c.y +  2.03211f * c.u/*+      0.f * c.v*/;
    ret.a = c.a;
    return clamp(ret);
}
/** https://en.wikipedia.org/wiki/YUV */
C4_CONST C4_ALWAYS_INLINE fcolor3 to_rgb_sdtv(yuv3 c) noexcept
{
    fcolor3 ret;
    ret.r = c.y + /*    0.f * c.u+*/ 1.13983f * c.v;
    ret.g = c.y + -0.39465f * c.u + -0.58060f * c.v;
    ret.b = c.y +  2.03211f * c.u/*+      0.f * c.v*/;
    return clamp(ret);
}

/** https://en.wikipedia.org/wiki/YUV */
C4_CONST C4_ALWAYS_INLINE yuv to_yuv_hdtv(fcolor c) noexcept
{
    yuv ret;
    ret.y =  0.21260f * c.r +  0.71520f * c.g +  0.07720f * c.b;
    ret.u = -0.09991f * c.r + -0.33609f * c.g +  0.43600f * c.b;
    ret.v =  0.61500f * c.r + -0.55861f * c.g + -0.05639f * c.b;
    ret.a = c.a;
    return ret;
}
/** https://en.wikipedia.org/wiki/YUV */
C4_CONST C4_ALWAYS_INLINE yuv3 to_yuv_hdtv(fcolor3 c) noexcept
{
    yuv3 ret;
    ret.y =  0.21260f * c.r +  0.71520f * c.g +  0.07720f * c.b;
    ret.u = -0.09991f * c.r + -0.33609f * c.g +  0.43600f * c.b;
    ret.v =  0.61500f * c.r + -0.55861f * c.g + -0.05639f * c.b;
    return ret;
}
C4_CONST C4_ALWAYS_INLINE float to_luminance_hdtv(fcolor c) noexcept
{
    return 0.21260f * c.r + 0.71520f * c.g + 0.07720f * c.b;
}
C4_CONST C4_ALWAYS_INLINE float to_luminance_hdtv(fcolor3 c) noexcept
{
    return 0.21260f * c.r + 0.71520f * c.g + 0.07720f * c.b;
}

/** https://en.wikipedia.org/wiki/YUV */
C4_CONST C4_ALWAYS_INLINE fcolor to_rgb_hdtv(yuv c) noexcept
{
    fcolor ret;
    ret.r = c.y + /*    0.f * c.u+*/ 1.28033f * c.v;
    ret.g = c.y + -0.21482f * c.u + -0.38059f * c.v;
    ret.b = c.y +  2.12798f * c.u/*+      0.f * c.v*/;
    ret.a = c.a;
    return clamp(ret);
}
/** https://en.wikipedia.org/wiki/YUV */
C4_CONST C4_ALWAYS_INLINE fcolor3 to_rgb_hdtv(yuv3 c) noexcept
{
    fcolor3 ret;
    ret.r = c.y + /*    0.f * c.u+*/ 1.28033f * c.v;
    ret.g = c.y + -0.21482f * c.u + -0.38059f * c.v;
    ret.b = c.y +  2.12798f * c.u/*+      0.f * c.v*/;
    return clamp(ret);
}

} // namespace quickgui

#endif /* QUICKGUI_YUV_HPP_ */
