#ifndef QUICKGUI_IMGVIEW_HPP_
#define QUICKGUI_IMGVIEW_HPP_

#include <cstddef>
#include <cstring>
#include <c4/error.hpp>


namespace quickgui {


// data view for a tightly packed image
struct imgview
{
    typedef enum {
        data_u8 = 0, // space the values by 4 channels
        data_i8 = 4,
        data_u32 = 8,
        data_i32 = 12,
        data_f32 = 16,
        // nothing else is supported
    } data_type_e;

    size_t data_type_size() const noexcept { return data_type_size(data_type); }

    static size_t data_type_size(data_type_e dt) noexcept
    {
        switch(dt)
        {
        case data_u8:
        case data_i8:
            return 1u;
        case data_u32:
        case data_i32:
        case data_f32:
            return 4u;
        }
        C4_ERROR("unknown data type");
        return 0u;
    }

public:

    char * C4_RESTRICT buf = nullptr;
    size_t buf_size = 0;

    size_t width = 0;       // x, or number of columns
    size_t height = 0;      // y, or number of rows
    size_t pixel_area = 0;  // width * height // TODO turn to function
    size_t num_channels = 0;
    size_t num_bytes_per_channel = 0; // TODO turn to function
    size_t size = 0;        // width * height * num_channels // TODO turn to function
    size_t size_bytes = 0;  // width * height * num_channels * num_bytes_per_channel // TODO turn to function

    data_type_e data_type = data_u32;

public:

    void reset(char *buf, size_t sz, size_t width, size_t height,
               size_t num_channels, imgview::data_type_e data_type);

    void load_bmp(char *buf, size_t bufsz);
    size_t save_bmp(char *buf, size_t bufsz);

public:

    operator bool() const noexcept { return buf != nullptr; }
    bool is_same(imgview const& that) const noexcept
    {
        return buf == that.buf
            && width == that.width
            && height == that.height
            && pixel_area == that.pixel_area
            && num_channels == that.num_channels
            && num_bytes_per_channel == that.num_bytes_per_channel
            && size == that.size
            && size_bytes == that.size_bytes
            && data_type == that.data_type;
    }

    C4_ALWAYS_INLINE size_t pos(size_t w, size_t h, size_t ch) const noexcept
    {
        C4_ASSERT(w < width);
        C4_ASSERT(h < height);
        C4_ASSERT(ch < num_channels);
        return ch + num_channels * (h * width + w);
    }

    template<class T>
    T get(size_t w, size_t h, size_t ch) const noexcept
    {
        C4_XASSERT(sizeof(T) == num_bytes_per_channel);
        C4_XASSERT(std::is_integral_v<T> == ((data_type == data_u8) || (data_type == data_i8) || (data_type == data_u32) || (data_type == data_i32)));
        C4_XASSERT(std::is_signed_v<T> == ((data_type == data_i8) || (data_type == data_i32) || (data_type == data_f32)));
        C4_XASSERT(std::is_floating_point_v<T> == (data_type == data_f32));
        C4_XASSERT(buf != nullptr);
        const size_t p = pos(w, h, ch);
        C4_XASSERT(p * sizeof(T) < buf_size);
        C4_XASSERT(p < size);
        T const *C4_RESTRICT const arr = reinterpret_cast<T const *>(buf);
        return arr[p];
    }

    template<class T>
    void set(size_t w, size_t h, size_t ch, T chval) const noexcept
    {
        C4_XASSERT(sizeof(T) == num_bytes_per_channel);
        C4_XASSERT(std::is_integral_v<T> == ((data_type == data_u8) || (data_type == data_i8) || (data_type == data_u32) || (data_type == data_i32)));
        C4_XASSERT(std::is_signed_v<T> == ((data_type == data_i8) || (data_type == data_i32) || (data_type == data_f32)));
        C4_XASSERT(std::is_floating_point_v<T> == (data_type == data_f32));
        C4_XASSERT(buf != nullptr);
        const size_t p = pos(w, h, ch);
        C4_XASSERT(p * sizeof(T) < buf_size);
        C4_XASSERT(p < size);
        T *C4_RESTRICT arr = reinterpret_cast<T *>(buf);
        arr[p] = chval;
    }

};


C4_SUPPRESS_WARNING_MSVC_WITH_PUSH(4702)  // unreachable code
template<class T>
imgview::data_type_e make_data_type()
{
    if constexpr(std::is_same_v<T, uint8_t>)
        return imgview::data_u8;
    else if constexpr(std::is_same_v<T, int8_t>)
        return imgview::data_i8;
    if constexpr(std::is_same_v<T, uint32_t>)
        return imgview::data_u32;
    else if constexpr(std::is_same_v<T, int32_t>)
        return imgview::data_i32;
    else if constexpr(std::is_same_v<T, float>)
        return imgview::data_f32;
    C4_NOT_IMPLEMENTED();
    C4_UNREACHABLE();
    return imgview::data_u32;
}
C4_SUPPRESS_WARNING_MSVC_POP


imgview make_view(char *buf, size_t sz, size_t width, size_t height,
                  size_t num_channels, imgview::data_type_e data_type);


imgview load_bmp(char *buf, size_t bufsz);


namespace detail {
/* example:
imgview load(const char *filename, char *buf, size_t bufsz);

template<class LinearContainerOfChar>
imgview load(const char *filename, LinearContainerOfChar *cont)
{
    return detail::_resize_container_and_create_imgview(cont, [&](char *dat, size_t sz){
        return load(filename, dat, sz);
    });
}
*/
template<class LinearContainerOfChar, class Function>
imgview _resize_container_and_create_imgview(LinearContainerOfChar *cont, Function &&fn)
{
    imgview view = cont->empty() ? fn(nullptr, 0) : fn(cont->data(), cont->size());
    cont->resize(view.size_bytes);
    if(!view)
    {
       view = fn(cont->data(), cont->size());
       cont->resize(view.size_bytes);
    }
    return view;
}
} // namespace detail


void vflip(imgview const& C4_RESTRICT src, imgview & C4_RESTRICT dst) noexcept;
void convert_channels(imgview const& C4_RESTRICT src, imgview & C4_RESTRICT dst) noexcept;

} // namespace quickgui

#endif // QUICKGUI_IMGVIEW_HPP_
