#include "quickgui/imgview.hpp"
#include "quickgui/mem.hpp"
#include "quickgui/math.hpp"
#include <c4/types.hpp>
#include <c4/error.hpp>
#include <c4/format.hpp>
#include <c4/memory_util.hpp>
#include <cstring>


C4_SUPPRESS_WARNING_GCC_CLANG_PUSH
C4_SUPPRESS_WARNING_CLANG("-Wcast-align")
C4_SUPPRESS_WARNING_GCC_CLANG("-Wold-style-cast")
C4_SUPPRESS_WARNING_GCC("-Wuseless-cast")

namespace quickgui {

namespace {

// https://solarianprogrammer.com/2018/11/19/cpp-reading-writing-bmp-images/
#pragma pack(push, 1)
struct BMPFileHeader
{
    uint16_t file_type{0x4d42};            // File type always BM which is 0x4D42 (stored as hex uint16_t in little endian)
    uint32_t file_size{0};                 // Size of the file (in bytes)
    uint16_t reserved1{0};                 // Reserved, always 0
    uint16_t reserved2{0};                 // Reserved, always 0
    uint32_t offset_data{0};               // Start position of pixel data (bytes from the beginning of the file)
};
static_assert(sizeof(BMPFileHeader) == 14u);

struct BMPInfoHeader
{
    uint32_t size{0};                      // Size of this header (in bytes)
    int32_t width{0};                      // width of bitmap in pixels
    int32_t height{0};                     // width of bitmap in pixels
                                           //       (if positive, bottom-up, with origin in lower left corner)
                                           //       (if negative, top-down, with origin in upper left corner)
    uint16_t planes{1};                    // No. of planes for the target device, this is always 1
    uint16_t bit_count{0};                 // No. of bits per pixel
    uint32_t compression{0};               // Compression mask.
    uint32_t size_image{0};                // 0 - for uncompressed images
    int32_t x_pixels_per_meter{0};
    int32_t y_pixels_per_meter{0};
    uint32_t colors_used{0};               // No. color indexes in the color table. Use 0 for the max number of colors allowed by bit_count
    uint32_t colors_important{0};          // No. of colors used for displaying the bitmap. If 0 all colors are required
};
static_assert(sizeof(BMPInfoHeader) == 40u);

struct BMPColorHeader
{
    uint32_t red_mask  {0x00ff0000};       // Bit mask for the red channel
    uint32_t green_mask{0x0000ff00};       // Bit mask for the green channel
    uint32_t blue_mask {0x000000ff};       // Bit mask for the blue channel
    uint32_t alpha_mask{0xff000000};       // Bit mask for the alpha channel
    uint32_t color_space_type{0x73524742}; // Default "sRGB" (0x73524742)
    uint32_t unused[16]{0};                // Unused data for sRGB color space
};
#pragma pack(pop)
} // anon namespace


wimgview load_bmp(void * bmp_buf_, uint32_t bmp_buf_sz)
{
    char *C4_RESTRICT bmp_buf = (char *)bmp_buf_;
    BMPFileHeader const* C4_RESTRICT file_header = (BMPFileHeader const*)(bmp_buf);
    C4_CHECK(file_header->file_type == 0x4d42);
    C4_CHECK(file_header->file_size <= (uint32_t)bmp_buf_sz);
    C4_CHECK(file_header->offset_data < (uint32_t)bmp_buf_sz);
    BMPInfoHeader const* C4_RESTRICT info_header = (BMPInfoHeader const*)(bmp_buf + sizeof(BMPFileHeader));
    C4_CHECK(info_header->size == 40);
    C4_CHECK(info_header->size == sizeof(BMPInfoHeader));
    C4_CHECK(info_header->compression == 0); // check not compressed
    C4_CHECK((info_header->bit_count % 8) == 0);
    C4_CHECK(file_header->offset_data >= sizeof(BMPFileHeader) + info_header->size);
    C4_CHECK(file_header->offset_data >= sizeof(BMPFileHeader) + sizeof(BMPInfoHeader));
    if(file_header->offset_data >= sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + sizeof(BMPColorHeader))
    {
        C4_NOT_IMPLEMENTED(); // FIXME
        BMPColorHeader const* C4_RESTRICT color_header = nullptr;
        color_header = (BMPColorHeader const*)(bmp_buf + sizeof(BMPFileHeader) + sizeof(BMPInfoHeader));
        C4_CHECK(color_header->red_mask == 0x0000ff00);
        C4_CHECK(color_header->red_mask == 0x000000ff);
        C4_CHECK(color_header->green_mask == 0x0000ff00);
        C4_CHECK(color_header->blue_mask == 0x00ff0000);
        C4_CHECK(color_header->alpha_mask == 0xff000000);
    }
    const bool inverted = info_header->height < 0;
    const uint32_t height = (uint32_t)(inverted ? -info_header->height : info_header->height);
    wimgview v = make_wimgview(
        /*buf*/bmp_buf + file_header->offset_data,
        /*bufsz*/(uint32_t)(bmp_buf_sz - (size_t)file_header->offset_data),
        /*width*/(uint32_t)info_header->width,
        /*height*/height,
        /*num_channels*/info_header->bit_count / 8u,
        /*data_type*/imgviewtype::u8);
    // remove any padding at the end of the rows
    C4_ASSERT((v.width & 3) == 0);
    // flip in place if required
    if(inverted)
        vflip(v);
    return v;
}


size_t save_bmp(imgview const& C4_RESTRICT v, char *bmp_buf, size_t bmp_buf_sz)
{
    size_t bmp_buf_pos = 0;
    auto _nextfield = [&](size_t sz){
        char *field = bmp_buf + bmp_buf_pos;
        bmp_buf_pos += sz;
        return bmp_buf && bmp_buf_pos <= bmp_buf_sz ? field : nullptr;
    };
    BMPFileHeader *file_header = nullptr;
    BMPInfoHeader *info_header = nullptr;
    BMPColorHeader *color_header = nullptr;
    uint16_t bit_count = (uint16_t)(v.num_channels * v.num_bytes_per_channel() * size_t(8));
    if(char *field = _nextfield(sizeof(BMPFileHeader)); field)
    {
        file_header = (BMPFileHeader*)field;
        *file_header = BMPFileHeader{};
        C4_CHECK(file_header->file_type == 0x4d42);
    }
    if(char *field = _nextfield(sizeof(BMPInfoHeader)); field)
    {
        info_header = (BMPInfoHeader*)field;
        *info_header = BMPInfoHeader{};
        info_header->size = (uint32_t)sizeof(BMPInfoHeader);
        info_header->width = (int32_t)v.width;
        info_header->height = (int32_t)v.height;
        info_header->bit_count = bit_count;
        info_header->size_image = v.bytes_required();
    }
    if(bit_count == 32)
    {
        if(char *field = _nextfield(sizeof(BMPColorHeader)); field)
        {
            color_header = (BMPColorHeader*)field;
            *color_header = BMPColorHeader{};
            C4_CHECK(info_header);
            info_header->size += (uint32_t)sizeof(BMPColorHeader);
        }
    }
    // write the table, with 1024 bytes
    enum : size_t { skip_bytes = 1024 };
    if(char *field = _nextfield(skip_bytes); field)
    {
        size_t pos = 0;
        for(size_t i = 0; i < 256; ++i)
        {
            const char c = (char)i;
            field[pos++] = c;
            field[pos++] = c;
            field[pos++] = c;
            field[pos++] = '\0';
        }
        C4_ASSERT(pos == skip_bytes);
    }
    uint32_t offset_data = (uint32_t)bmp_buf_pos;
    size_t num_bytes = v.bytes_required();
    C4_ASSERT(num_bytes <= v.buf_size);
    if(char *field = _nextfield(num_bytes); field)
    {
        if(field != (void const*)v.buf)
        {
            C4_ASSERT(!c4::mem_overlaps(field, v.buf, num_bytes, num_bytes));
            memcpy(field, v.buf, num_bytes);
        }
    }
    if(file_header)
    {
        file_header->offset_data = offset_data;
        file_header->file_size = offset_data + (uint32_t)num_bytes;
    }
    return bmp_buf_pos;
}


//-----------------------------------------------------------------------------

wimgview load_pfm(void * bmp_buf_, uint32_t bmp_buf_sz)
{
    // https://www.pauldebevec.com/Research/HDR/PFM/
    char *C4_RESTRICT bmp_buf = (char *)bmp_buf_;
    C4_CHECK(bmp_buf_sz > 3u);
    c4::csubstr buf(bmp_buf, bmp_buf_sz);
    uint32_t num_channels = {};
    if(buf.begins_with("PF"))
        num_channels = 3u;
    else if(buf.begins_with("Pf"))
        num_channels = 1u;
    else
        C4_ERROR("unknown pfm type");
    C4_CHECK(buf[2] == '\r');
    // done with the first line
    buf = buf.sub(3);
    size_t nextcr = buf.first_of("\r");
    C4_CHECK(nextcr != c4::csubstr::npos);
    c4::csubstr second_line = buf.first(nextcr);
    C4_CHECK(second_line.len > 0);
    C4_CHECK(second_line.len < buf.len);
    C4_CHECK(!second_line.ends_with('\r'));
    uint32_t width = {};
    uint32_t height = {};
    char space = {};
    size_t numchars = c4::uncat(second_line, width, space, height);
    C4_CHECK(numchars == second_line.len);
    C4_CHECK(width > 0);
    C4_CHECK(height > 0);
    const size_t num_bytes = (size_t)width * (size_t)height * (size_t)num_channels * sizeof(float);
    // done with the second line
    buf = buf.sub(nextcr + 1u);
    nextcr = buf.first_of("\r");
    C4_CHECK(nextcr != c4::csubstr::npos);
    c4::csubstr third_line = buf.first(nextcr);
    float endian_number = {};
    numchars = c4::uncat(third_line, endian_number);
    C4_CHECK(numchars == third_line.len);
    const bool little_endian = endian_number < 0.f;
    C4_NOT_IMPLEMENTED_IF(!little_endian);
    // done with the third line
    buf = buf.sub(nextcr + 1u);
    C4_CHECK(bmp_buf_sz > buf.len);
    C4_CHECK(buf.len >= num_bytes);
    wimgview v = make_wimgview(
        /*buf*/(char*)buf.str,
        /*bufsz*/(uint32_t)(bmp_buf_sz - buf.len),
        /*width*/width,
        /*height*/height,
        /*num_channels*/num_channels,
        /*data_type*/imgviewtype::f32);
    // the data is specified in left to right, bottom to top order.
    // flip in place
    vflip(v);
    return v;
}

size_t save_pfm(imgview const& C4_RESTRICT v, char *bmp_buf, size_t bmp_buf_sz)
{
    C4_NOT_IMPLEMENTED();
    return 0;
}


//-----------------------------------------------------------------------------


imgview make_imgview(void const* buf, uint32_t sz, imgview const& blueprint)
{
    imgview v;
    v.reset((imgview::buffer_type*)buf, sz, blueprint.width, blueprint.height, blueprint.num_channels, blueprint.data_type);
    return v;
}

wimgview make_wimgview(void *buf, uint32_t sz, imgview const& blueprint)
{
    wimgview v;
    v.reset((wimgview::buffer_type*)buf, sz, blueprint.width, blueprint.height, blueprint.num_channels, blueprint.data_type);
    return v;
}

wimgview make_wimgview(void *buf, uint32_t sz, uint32_t width, uint32_t height,
                       uint32_t num_channels, imgview::data_type_e dt)
{
    wimgview v;
    v.reset((wimgview::buffer_type*)buf, sz, width, height, num_channels, dt);
    return v;
}
imgview make_imgview(void const* buf, uint32_t sz, uint32_t width, uint32_t height,
                     uint32_t num_channels, imgview::data_type_e dt)
{
    imgview v;
    v.reset((imgview::buffer_type*)buf, sz, width, height, num_channels, dt);
    return v;
}


//-----------------------------------------------------------------------------

void check_img(imgview const& C4_RESTRICT img)
{
    C4_CHECK(img.buf != NULL);
    C4_CHECK(img.bytes_required() <= img.buf_size);
}
void check_pair(imgview const& C4_RESTRICT src, imgview const& C4_RESTRICT dst)
{
    check_img(src);
    check_img(dst);
    C4_CHECK(src.buf != dst.buf);
    C4_CHECK(!c4::mem_overlaps(src.buf, dst.buf, src.bytes_required(), dst.bytes_required()));
    C4_CHECK(src.width == dst.width);
    C4_CHECK(src.height == dst.height);
    C4_CHECK(src.num_channels == dst.num_channels);
    C4_CHECK(src.data_type == dst.data_type);
    C4_CHECK(src.bytes_required() <= src.buf_size);
    C4_CHECK(dst.bytes_required() <= dst.buf_size);
}

wimgview copy_img(imgview const& C4_RESTRICT src, void *buf, uint32_t bufsz) noexcept
{
    wimgview dst = make_wimgview(buf, bufsz, src);
    C4_CHECK(dst.buf != NULL);
    check_pair(src, dst);
    memcpy(dst.buf, src.buf, src.bytes_required());
    return dst;
}

void copy_img(imgview const& C4_RESTRICT src, wimgview & C4_RESTRICT dst) noexcept
{
    dst = copy_img(src, dst.buf, dst.buf_size);
}


void vflip(imgview const& C4_RESTRICT src, wimgview & C4_RESTRICT dst) noexcept
{
    check_pair(src, dst);
    C4_CHECK(!c4::mem_overlaps(src.buf, dst.buf, src.bytes_required(), dst.bytes_required()));
    using T = int8_t;
    using I = int32_t;
    const I H = (I)src.height;
    const I N = (I)src.width * (I)src.num_channels * (I)src.data_type_size();
    const uint32_t sN = (uint32_t)N;
    T const* C4_RESTRICT src_buf = (T const*) src.buf;
    T      * C4_RESTRICT dst_buf = (T      *) dst.buf;
    for(I h = 0; h < H; ++h)
    {
        T const* C4_RESTRICT src_row = src_buf +          h  * N;
        T      * C4_RESTRICT dst_row = dst_buf + (H - 1 - h) * N;
        memcpy(dst_row, src_row, sN);
    }
}

void vflip(wimgview & C4_RESTRICT dst) noexcept
{
    using T = uint8_t;
    using I = int32_t;
    const I H = (I)dst.height;
    const I H2 = (I)H/(I)2;
    const I W = (I)dst.width;
    const I N = W * (I)dst.num_channels * (I)dst.data_type_size();
    T* C4_RESTRICT dst_buf = (T*) dst.buf;
    for(I h = 0; h < H2; ++h)
    {
        T * C4_RESTRICT src_row = dst_buf +          h  * N;
        T * C4_RESTRICT dst_row = dst_buf + (H - 1 - h) * N;
        for(I w = 0; w < N; ++w)
        {
            uint8_t tmp = src_row[w];
            src_row[w] = dst_row[w];
            dst_row[w] = tmp;
        }
    }
}

void swaprb(wimgview & C4_RESTRICT dst) noexcept
{
    C4_CHECK(dst.num_channels == 3u);
    C4_CHECK(dst.data_type == imgviewtype::u8);
    using T = uint8_t;
    using I = int32_t;
    const I H = (I)dst.height;
    const I W = (I)dst.width;
    const I A = W * H;
    C4_CHECK((I)dst.buf_size >= I(3) * A);
    T* C4_RESTRICT dst_buf = (T*) dst.buf;
    for(I i = 0; i < A; ++i)
    {
        T * C4_RESTRICT pxvals = dst_buf + 3 * i;
        const T tmp = pxvals[0];
        pxvals[0] = pxvals[2];
        pxvals[2] = tmp;
    }
}

void convert_channels(imgview const& C4_RESTRICT src, wimgview & C4_RESTRICT dst) noexcept
{
    C4_CHECK(src.data_type == imgviewtype::u8);
    C4_CHECK(dst.data_type == imgviewtype::u8);
    C4_CHECK(src.buf != dst.buf);
    C4_CHECK(src.width == dst.width);
    C4_CHECK(src.height == dst.height);
    C4_CHECK(src.buf_size / src.num_channels == dst.buf_size / dst.num_channels);
    C4_CHECK(!c4::mem_overlaps(src.buf, dst.buf, src.bytes_required(), dst.bytes_required()));
    using T = int8_t;
    using I = int32_t;
    const I H = (I)src.height;
    const I W = (I)src.width;
    enum : T { alphamax = -1 };
    T const* C4_RESTRICT src_buf = (T const*) src.buf;
    T      * C4_RESTRICT dst_buf = (T      *) dst.buf;
    #define iterview(...)                  \
        for(I h = 0; h < H; ++h)           \
        {                                  \
            for(I w = 0; w < W; ++w)       \
            {                              \
                const I pos = (h * W + w); \
                __VA_ARGS__                \
            }                              \
        }
    switch(src.num_channels)
    {
    case 1:
    {
        if(dst.num_channels == 3)
        {
            iterview(
                dst_buf[pos * 3    ] = src_buf[pos];
                dst_buf[pos * 3 + 1] = src_buf[pos];
                dst_buf[pos * 3 + 2] = src_buf[pos];
            )
        }
        else if(dst.num_channels == 4)
        {
            iterview(
                dst_buf[pos * 4    ] = src_buf[pos];
                dst_buf[pos * 4 + 1] = src_buf[pos];
                dst_buf[pos * 4 + 2] = src_buf[pos];
                dst_buf[pos * 4 + 3] = alphamax;
            )
        }
        else
        {
            C4_NOT_IMPLEMENTED();
        }
        break;
    }
    case 3:
    {
        if(dst.num_channels == 1)
        {
            iterview(
                dst_buf[pos] = src_buf[pos * 3];
            )
        }
        else if(dst.num_channels == 4)
        {
            iterview(
                dst_buf[pos * 4    ] = src_buf[pos * 3];
                dst_buf[pos * 4 + 1] = src_buf[pos * 3 + 1];
                dst_buf[pos * 4 + 2] = src_buf[pos * 3 + 2];
                dst_buf[pos * 4 + 3] = alphamax;
            )
        }
        else
        {
            C4_NOT_IMPLEMENTED();
        }
        break;
    }
    case 4:
    {
        if(dst.num_channels == 1)
        {
            iterview(
                dst_buf[pos] = src_buf[pos * 4];
            )
        }
        else if(dst.num_channels == 3)
        {
            iterview(
                dst_buf[pos * 3    ] = src_buf[pos * 4];
                dst_buf[pos * 3 + 1] = src_buf[pos * 4 + 1];
                dst_buf[pos * 3 + 2] = src_buf[pos * 4 + 2];
            )
        }
        else
        {
            C4_NOT_IMPLEMENTED();
        }
        break;
    }
    default:
        C4_NOT_IMPLEMENTED();
    }
    #undef iterview
}


void hshift(imgview const& C4_RESTRICT src, wimgview &C4_RESTRICT dst, int32_t offs) noexcept
{
    copy_img(src, dst);
    check_pair(src, dst);
    hshift(dst, offs);
}
void vshift(imgview const& C4_RESTRICT src, wimgview &C4_RESTRICT dst, int32_t offs) noexcept
{
    copy_img(src, dst);
    check_pair(src, dst);
    vshift(dst, offs);
}
void hshift(wimgview & C4_RESTRICT img, int32_t offs) noexcept
{
    check_img(img);
    if(!offs)
        return;
    const size_t bytes_pixel = img.num_channels * img.num_bytes_per_channel();
    const size_t bytes_row = (size_t)img.width * bytes_pixel;
    static_assert(sizeof(img.buf[0]) == 1u);
    uint8_t *buf = (uint8_t*)img.buf;
    if(offs > 0)
    {
        C4_CHECK((uint32_t)offs <= img.width);
        const size_t bytes_offs = (size_t)offs * bytes_pixel;
        const size_t bytes_copy = bytes_row - bytes_offs;
        for(uint32_t row = 0; row < img.height; ++row)
        {
            uint8_t const* src = buf + row * bytes_row;
            uint8_t /* */* dst = buf + row * bytes_row + bytes_offs;
            memmove(dst, src, bytes_copy);
        }
    }
    else
    {
        C4_CHECK((uint32_t)(-offs) <= img.width);
        const size_t bytes_offs = (size_t)(offs) * bytes_pixel;
        const size_t bytes_copy = bytes_row - bytes_offs;
        uint8_t *buf = (uint8_t*)img.buf;
        for(uint32_t row = 0; row < img.height; ++row)
        {
            uint8_t /* */* dst = buf + row * bytes_row;
            uint8_t const* src = dst + bytes_offs;
            memmove(dst, src, bytes_copy);
        }
    }
}
void vshift(wimgview & C4_RESTRICT img, int32_t offs) noexcept
{
    check_img(img);
    if(!offs)
        return;
    const size_t bytes_pixel = img.num_channels * img.num_bytes_per_channel();
    const size_t bytes_row = (size_t)img.width * bytes_pixel;
    static_assert(sizeof(img.buf[0]) == 1u);
    uint8_t *buf = (uint8_t*)img.buf;
    if(offs > 0)
    {
        const uint32_t uoffs = (uint32_t)offs;
        C4_CHECK(uoffs <= img.width);
        uint8_t const* src = buf;
        uint8_t /* */* dst = buf + uoffs * bytes_row;
        memmove(dst, src, bytes_row);
    }
    else
    {
        const uint32_t uoffs = (uint32_t)-offs;
        C4_CHECK(uoffs <= img.width);
        uint8_t const* src = buf + uoffs * bytes_row;
        uint8_t /* */* dst = buf;
        memmove(dst, src, bytes_row);
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void convert_yuyv422_to_rgb(yuy2view const& src, wimgview const& dst)
{
    C4_CHECK(dst.data_type == imgviewtype::u8);
    C4_CHECK(src.buf != dst.buf);
    C4_CHECK(src.width == dst.width);
    C4_CHECK(src.height == dst.height);
    C4_CHECK(src.pixel_area() == dst.pixel_area());
    C4_CHECK(!c4::mem_overlaps(src.buf, dst.buf, src.bytes_required(), dst.bytes_required()));
    // https://stackoverflow.com/questions/72056909/convert-yuv2-yuyv-frames-to-rgb-without-use-of-opencv
    enum : int {
        Y_OFFSET = 16,
        UV_OFFSET = 128,
        YUV2RGB_11 = 298,
        YUV2RGB_12 = -1,
        YUV2RGB_13 = 409,
        YUV2RGB_22 = -100,
        YUV2RGB_23 = -210,
        YUV2RGB_32 = 519,
        YUV2RGB_33 = 0,
        chR = 0,
        chG = 1,
        chB = 2,
        chY0 = 0,
        chY1 = 2,
        chU = 1,
        chV = 3,
    };
    using T = uint8_t;
    using I = int32_t;
    T const* C4_RESTRICT yuy2 = (T const*) src.buf;
    T      * C4_RESTRICT rgb = (T *) dst.buf;
    const I num_yuy_blocks = (I)src.pixel_area() / (I)2;
    for(I px = 0u; px < num_yuy_blocks; ++px)
    {
        const int u = ((int)yuy2[chU]) - UV_OFFSET;
        const int v = ((int)yuy2[chV]) - UV_OFFSET;
        const int uv_r = YUV2RGB_12 * u + YUV2RGB_13 * v;
        const int uv_g = YUV2RGB_22 * u + YUV2RGB_23 * v;
        const int uv_b = YUV2RGB_32 * u + YUV2RGB_33 * v;
        // 1st pixel
        int y = YUV2RGB_11 * (((int)yuy2[chY0]) - Y_OFFSET);
        rgb[chR] = (T)clamp<int, 0, 255>((y + uv_r) >> 8); // r
        rgb[chG] = (T)clamp<int, 0, 255>((y + uv_g) >> 8); // g
        rgb[chB] = (T)clamp<int, 0, 255>((y + uv_b) >> 8); // b
        rgb += 3;
        // 2nd pixel
        y = YUV2RGB_11 * (((int)yuy2[chY1]) - Y_OFFSET);
        rgb[chR] = (T)clamp<int, 0, 255>((y + uv_r) >> 8); // r
        rgb[chG] = (T)clamp<int, 0, 255>((y + uv_g) >> 8); // g
        rgb[chB] = (T)clamp<int, 0, 255>((y + uv_b) >> 8); // b
        rgb += 3;
        yuy2 += 4;
    }
}

} // namespace quickgui

C4_SUPPRESS_WARNING_CLANG_POP
