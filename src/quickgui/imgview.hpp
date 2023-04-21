#ifndef QUICKGUI_IMGVIEW_HPP_
#define QUICKGUI_IMGVIEW_HPP_

#include <cstddef>
#include <cstring>
#include <c4/error.hpp>


namespace quickgui {

struct imgviewtype
{
    typedef enum {
        u8,
        i8,
        u16,
        u32,
        i32,
        f32,
        // nothing else is supported
    } data_type_e;

    static uint32_t data_size(data_type_e dt) noexcept
    {
        switch(dt)
        {
        case u8:
        case i8:
            return 1u;
        case u16:
            return 2u;
        case u32:
        case i32:
        case f32:
            return 4u;
        default:
            break;
        }
        C4_ERROR("unknown data type");
        return 0u;
    }

    template<class T>
    static data_type_e from() noexcept
    {
        if constexpr(std::is_same_v<T, uint8_t>)
            return u8;
        else if constexpr(std::is_same_v<T, int8_t>)
            return i8;
        else if constexpr(std::is_same_v<T, uint16_t>)
            return u16;
        if constexpr(std::is_same_v<T, uint32_t>)
            return u32;
        else if constexpr(std::is_same_v<T, int32_t>)
            return i32;
        else if constexpr(std::is_same_v<T, float>)
            return f32;
        else
            C4_STATIC_ERROR(T, "invalid type");
        return u8;
    }
};


/** data view for a tightly packed image */
template<class T>
struct basic_imgview
{
    using buffer_type = T;
    using data_type_e = imgviewtype::data_type_e;

    static_assert(sizeof(T) == 1u);

    /// convert automatically to const view
    inline operator basic_imgview<const T> const& () const noexcept { return *(basic_imgview<const T> const*)this; }

public:

    T* C4_RESTRICT buf = nullptr;
    uint32_t buf_size = 0;

    uint32_t width = 0;       // x, or number of columns
    uint32_t height = 0;      // y, or number of rows
    uint32_t num_channels = 0;
    data_type_e data_type = imgviewtype::u8;

public:

    C4_ALWAYS_INLINE uint32_t pixel_area() const noexcept { return width * height; }
    C4_ALWAYS_INLINE uint32_t num_values() const noexcept { return width * height * num_channels; }
    C4_ALWAYS_INLINE uint32_t bytes_required() const noexcept { return imgviewtype::data_size(data_type) * width * height * num_channels; }
    C4_ALWAYS_INLINE uint32_t data_type_size() const noexcept { return imgviewtype::data_size(data_type); }
    C4_ALWAYS_INLINE uint32_t num_bytes_per_channel() const { return imgviewtype::data_size(data_type); }

public:

    void reset(T *buf, uint32_t sz, uint32_t width, uint32_t height,
               uint32_t num_channels, data_type_e data_type);

public:

    operator bool() const noexcept { return buf != nullptr; }

    template<class U>
    bool is_same(basic_imgview<U> const& that) const noexcept
    {
        return buf == that.buf
            && width == that.width
            && height == that.height
            && num_channels == that.num_channels
            && data_type == that.data_type;
    }

    template<class U>
    bool has_same_params(basic_imgview<U> const& that) const noexcept
    {
        return width == that.width
            && height == that.height
            && num_channels == that.num_channels
            && data_type == that.data_type;
    }

public:

    C4_ALWAYS_INLINE uint32_t pxpos(uint32_t w, uint32_t h) const noexcept
    {
        C4_ASSERT(w < width);
        C4_ASSERT(h < height);
        return (h * width + w);
    }

    C4_ALWAYS_INLINE uint32_t pos(uint32_t w, uint32_t h) const noexcept
    {
        C4_ASSERT(w < width);
        C4_ASSERT(h < height);
        C4_ASSERT(num_channels == 1u);
        return (h * width + w);
    }

    C4_ALWAYS_INLINE uint32_t pos(uint32_t w, uint32_t h, uint32_t ch) const noexcept
    {
        C4_ASSERT(w < width);
        C4_ASSERT(h < height);
        C4_ASSERT(ch < num_channels);
        return ch + num_channels * (h * width + w);
    }

    #define _imgviewcheck(T)\
        C4_XASSERT(sizeof(T) == num_bytes_per_channel()); \
        C4_XASSERT(std::is_integral_v<T> == ((data_type == imgviewtype::u8) || (data_type == imgviewtype::i8) || (data_type == imgviewtype::u16) || (data_type == imgviewtype::u32) || (data_type == imgviewtype::i32))); \
        C4_XASSERT(std::is_signed_v<T> == ((data_type == imgviewtype::i8) || (data_type == imgviewtype::i32) || (data_type == imgviewtype::f32))); \
        C4_XASSERT(std::is_floating_point_v<T> == (data_type == imgviewtype::f32)) \

    template<class U>
    auto data_as() const noexcept
        -> std::conditional_t<std::is_const_v<T>, U const*, U*>
    {
        _imgviewcheck(U);
        C4_XASSERT(buf != nullptr);
        using rettype = std::conditional_t<std::is_const_v<T>, U const, U>;
        return reinterpret_cast<rettype*>(buf);
    }

    template<class C>
    auto color_data_as() const noexcept
        -> std::conditional_t<std::is_const_v<T>, C const*, C*>
    {
        C4_ASSERT(sizeof(C) == num_channels * num_bytes_per_channel());
        _imgviewcheck(typename C::value_type);
        C4_XASSERT(buf != nullptr);
        using rettype = std::conditional_t<std::is_const_v<T>, C const, C>;
        return reinterpret_cast<rettype*>(buf);
    }

    template<class U>
    U get(uint32_t w, uint32_t h, uint32_t ch) const noexcept
    {
        _imgviewcheck(U);
        C4_XASSERT(buf != nullptr);
        const uint32_t p = pos(w, h, ch);
        C4_XASSERT(p * sizeof(U) < buf_size);
        C4_XASSERT(p < num_values());
        U const *C4_RESTRICT const arr = reinterpret_cast<U const *>(buf);
        return arr[p];
    }

    template<class U>
    U get(uint32_t w, uint32_t h) const noexcept
    {
        _imgviewcheck(U);
        C4_XASSERT(num_channels == 1u);
        C4_XASSERT(buf != nullptr);
        const uint32_t p = pos(w, h);
        C4_XASSERT(p * sizeof(U) < buf_size);
        C4_XASSERT(p < num_values());
        U const *C4_RESTRICT const arr = reinterpret_cast<U const *>(buf);
        return arr[p];
    }

    template<class U, class V=T>
    auto set(uint32_t w, uint32_t h, uint32_t ch, T chval) const noexcept
        -> std::enable_if_t<!std::is_const_v<V>, void>
    {
        _imgviewcheck(U);
        C4_XASSERT(buf != nullptr);
        const uint32_t p = pos(w, h, ch);
        C4_XASSERT(p * sizeof(U) < buf_size);
        C4_XASSERT(p < num_values());
        U *C4_RESTRICT const arr = reinterpret_cast<U *>(buf);
        arr[p] = chval;
    }

    template<class U, class V=T>
    auto set(uint32_t w, uint32_t h, U chval) const noexcept
        -> std::enable_if_t<!std::is_const_v<V>, void>
    {
        _imgviewcheck(T);
        C4_XASSERT(num_channels == 1u);
        C4_XASSERT(buf != nullptr);
        const uint32_t p = pos(w, h);
        C4_XASSERT(p * sizeof(T) < buf_size);
        C4_XASSERT(p < num_values());
        U *C4_RESTRICT const arr = reinterpret_cast<U *>(buf);
        arr[p] = chval;
    }

    #undef _imgviewcheck
};


using wimgview = basic_imgview<uint8_t>;
using imgview = basic_imgview<const uint8_t>;


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

imgview make_imgview(void const* buf, uint32_t sz, imgview const& blueprint);
wimgview make_wimgview(void *buf, uint32_t sz, imgview const& blueprint);

imgview make_imgview(void const* buf, uint32_t sz, uint32_t width, uint32_t height,
                      uint32_t num_channels, imgview::data_type_e data_type);
wimgview make_wimgview(void *buf, uint32_t sz, uint32_t width, uint32_t height,
                      uint32_t num_channels, imgview::data_type_e data_type);


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

template<class CharContainer>
imgview make_imgview(CharContainer *container, imgview const& blueprint)
{
    static_assert(sizeof(typename CharContainer::value_type) == 1u);
    imgview dst = make_imgview(container->data(), (uint32_t)container->size(), blueprint);
    if(dst.bytes_required() > container->size())
    {
        container->resize(dst.bytes_required());
        dst = make_imgview(container->data(), (uint32_t)container->size(), blueprint);
    }
    return dst;
}
template<class CharContainer>
wimgview make_wimgview(CharContainer *container, imgview const& blueprint)
{
    static_assert(sizeof(typename CharContainer::value_type) == 1u);
    wimgview dst = make_wimgview(container->data(), (uint32_t)container->size(), blueprint);
    if(dst.bytes_required() > container->size())
    {
        container->resize(dst.bytes_required());
        dst = make_wimgview(container->data(), (uint32_t)container->size(), blueprint);
    }
    return dst;
}

template<class CharContainer>
imgview make_imgview(CharContainer *container, uint32_t width, uint32_t height,
                     uint32_t num_channels, imgview::data_type_e data_type)
{
    static_assert(sizeof(typename CharContainer::value_type) == 1u);
    imgview dst = make_imgview(container->data(), (uint32_t)container->size(), width, height, num_channels, data_type);
    if(dst.bytes_required() > container->size())
    {
        container->resize(dst.bytes_required());
        dst = make_imgview(container->data(), (uint32_t)container->size(), width, height, num_channels, data_type);
    }
    return dst;
}
template<class CharContainer>
wimgview make_wimgview(CharContainer *container, uint32_t width, uint32_t height,
                       uint32_t num_channels, imgview::data_type_e data_type)
{
    static_assert(sizeof(typename CharContainer::value_type) == 1u);
    wimgview dst = make_wimgview(container->data(), (uint32_t)container->size(), width, height, num_channels, data_type);
    if(dst.bytes_required() > container->size())
    {
        container->resize(dst.bytes_required());
        dst = make_wimgview(container->data(), (uint32_t)container->size(), width, height, num_channels, data_type);
    }
    return dst;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

wimgview load_bmp(void *buf, uint32_t bufsz);
uint32_t save_bmp(imgview const& C4_RESTRICT v, void *bmp_buf, uint32_t bmp_buf_sz);

void vflip(imgview const& C4_RESTRICT src, wimgview & C4_RESTRICT dst) noexcept;
void convert_channels(imgview const& C4_RESTRICT src, wimgview & C4_RESTRICT dst) noexcept;

} // namespace quickgui

#endif // QUICKGUI_IMGVIEW_HPP_
