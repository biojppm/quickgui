#ifndef QUICKGUI_MATH_HPP_
#define QUICKGUI_MATH_HPP_

#include <type_traits>
#include <cmath>
#include "quickgui/mem.hpp"

namespace quickgui {

template<class T> inline constexpr const T zero_v = T(0);
template<class T> inline constexpr const T one_v = T(1);


/** reciprocal of the square root */
template<class T>
C4_ALWAYS_INLINE C4_CONST constexpr T rsqrt(T val) noexcept
{
    return T(1) / std::sqrt(val);
}
/** reciprocal of the square root */
C4_ALWAYS_INLINE C4_CONST constexpr float rsqrt_approx(float val) noexcept
{
    // fast inverse square root:
    // https://en.wikipedia.org/wiki/Fast_inverse_square_root
    static_assert(std::numeric_limits<float>::is_iec559); // (enable only on IEEE 754)
	float const y = std::bit_cast<float>(0x5f3759df - (std::bit_cast<uint32_t>(val) >> 1));
	return y * (1.5f - (val * 0.5f * y * y));
}


/** linear interpolation */
template<class T>
C4_ALWAYS_INLINE C4_CONST constexpr auto lerp(T factor, T min, T max) noexcept -> std::enable_if_t<std::is_floating_point_v<T>, T>
{
    return (T(1) - factor) * min + factor * max;
}
/** linear interpolation for integral types. The factor needs to be a floating type. */
template<class F, class I>
C4_ALWAYS_INLINE C4_CONST constexpr auto lerp(F factor, I min, I max) noexcept -> std::enable_if_t<std::is_floating_point_v<F> && std::is_integral_v<I>, I>
{
    return static_cast<I>((F(1) - factor) * min + factor * max);
}

/** std::clamp uses references. Values are better for fundamental types. */
template<class T>
C4_ALWAYS_INLINE C4_CONST constexpr auto clamp01(T val) noexcept -> std::enable_if_t<std::is_fundamental_v<T>, T>
{
    if(C4_LIKELY(val >= T(0) && val <= T(1)))
        return val;
    else if(val > T(1))
        return T(1);
    return T(0);
}

/** std::clamp uses references. Values are better for fundamental types. */
template<class T>
C4_ALWAYS_INLINE C4_CONST constexpr auto clamp(T val, T min, T max) noexcept -> std::enable_if_t<std::is_fundamental_v<T>, T>
{
    if(C4_LIKELY(val >= min && val <= max))
        return val;
    else if(val > max)
        return max;
    return min;
}

/** std::max uses references. Values are better for fundamental types. */
template<class T>
C4_ALWAYS_INLINE C4_CONST constexpr auto max(T a, T b) noexcept -> std::enable_if_t<std::is_fundamental_v<T>, T>
{
    return a >= b ? a : b;
}

/** std::min uses references. Values are better for fundamental types. */
template<class T>
C4_ALWAYS_INLINE C4_CONST constexpr auto min(T a, T b) noexcept -> std::enable_if_t<std::is_fundamental_v<T>, T>
{
    return a <= b ? a : b;
}

/** std::max uses references. Values are better for fundamental types. */
template<class T>
C4_ALWAYS_INLINE C4_CONST constexpr auto max(T a, T b, T c) noexcept -> std::enable_if_t<std::is_fundamental_v<T>, T>
{
    return max(a >= b ? a : b, c);
}

/** std::min uses references. Values are better for fundamental types. */
template<class T>
C4_ALWAYS_INLINE C4_CONST constexpr auto min(T a, T b, T c) noexcept -> std::enable_if_t<std::is_fundamental_v<T>, T>
{
    return min(a <= b ? a : b, c);
}

/** std::max uses references. Values are better for fundamental types. */
template<class T>
C4_ALWAYS_INLINE C4_CONST constexpr auto max(T a, T b, T c, T d) noexcept -> std::enable_if_t<std::is_fundamental_v<T>, T>
{
    return max(a >= b ? a : b, c >= d ? c : d);
}

/** std::min uses references. Values are better for fundamental types. */
template<class T>
C4_ALWAYS_INLINE C4_CONST constexpr auto min(T a, T b, T c, T d) noexcept -> std::enable_if_t<std::is_fundamental_v<T>, T>
{
    return min(a <= b ? a : b, c <= d ? c : d);
}

} // namespace quickgui

#endif /* QUICKGUI_MATH_HPP_ */
