#include "quickgui/imgview.hpp"
#include "quickgui/mem.hpp"
#include <c4/types.hpp>
#include <c4/error.hpp>
#include <c4/memory_util.hpp>
#include <cstring>


C4_SUPPRESS_WARNING_CLANG_PUSH
C4_SUPPRESS_WARNING_CLANG("-Wcast-align")

namespace quickgui {

namespace {

// https://solarianprogrammer.com/2018/11/19/cpp-reading-writing-bmp-images/
#pragma pack(push, 1)
struct BMPFileHeader
{
    uint16_t file_type{0x4D42};            // File type always BM which is 0x4D42 (stored as hex uint16_t in little endian)
    uint32_t file_size{0};                 // Size of the file (in bytes)
    uint16_t reserved1{0};                 // Reserved, always 0
    uint16_t reserved2{0};                 // Reserved, always 0
    uint32_t offset_data{0};               // Start position of pixel data (bytes from the beginning of the file)
};

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

struct BMPColorHeader
{
    uint32_t red_mask{0x00ff0000};         // Bit mask for the red channel
    uint32_t green_mask{0x0000ff00};       // Bit mask for the green channel
    uint32_t blue_mask{0x000000ff};        // Bit mask for the blue channel
    uint32_t alpha_mask{0xff000000};       // Bit mask for the alpha channel
    uint32_t color_space_type{0x73524742}; // Default "sRGB" (0x73524742)
    uint32_t unused[16]{0};                // Unused data for sRGB color space
};
#pragma pack(pop)
} // anon namespace


void imgview::load_bmp(char * bmp_buf, size_t bmp_buf_sz)
{
    BMPFileHeader const* C4_RESTRICT file_header = (BMPFileHeader const*)(bmp_buf);
    C4_CHECK(file_header->file_type == 0x4d42);
    C4_CHECK(file_header->file_size <= (uint32_t)bmp_buf_sz);
    C4_CHECK(file_header->offset_data < (uint32_t)bmp_buf_sz);
    BMPInfoHeader const* C4_RESTRICT info_header = (BMPInfoHeader const*)(bmp_buf + sizeof(BMPFileHeader));
    if(info_header->bit_count != 32)
    {
        C4_CHECK(info_header->size >= sizeof(BMPInfoHeader));
    }
    else
    {
        BMPColorHeader const* C4_RESTRICT color_header = nullptr;
        C4_CHECK(info_header->size >= sizeof(BMPInfoHeader) + sizeof(BMPColorHeader));
        color_header = (BMPColorHeader const*)(bmp_buf + sizeof(BMPFileHeader) + sizeof(BMPInfoHeader));
        (void)color_header;
    }
    C4_CHECK(info_header->height > 0);
    buf = bmp_buf + file_header->offset_data;
    buf_size = bmp_buf_sz - file_header->offset_data;
    width = (uint32_t)info_header->width;
    height = (uint32_t)info_header->height;
    pixel_area = width * height;
    num_channels = info_header->bit_count / 8;
    num_bytes_per_channel = 1;
    size = pixel_area * num_channels;
    size_bytes = size * num_bytes_per_channel;
    data_type = imgview::data_u8;
    C4_CHECK(size_bytes == width * height * num_channels);
    C4_CHECK(size_bytes <= buf_size);
    // remove any padding at the end of the rows
    C4_ASSERT((width & 3) == 0);
}

size_t imgview::save_bmp(char *bmp_buf, size_t bmp_buf_sz)
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
    uint16_t bit_count = (uint16_t)(num_channels * num_bytes_per_channel * size_t(8));
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
        info_header->width = (int32_t)width;
        info_header->height = (int32_t)height;
        info_header->bit_count = bit_count;
        info_header->size_image = (uint32_t)size_bytes;
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
    if(char *field = _nextfield(this->size_bytes); field)
    {
        C4_ASSERT(this->size_bytes <= this->buf_size);
        if(field != this->buf)
        {
            C4_ASSERT(!c4::mem_overlaps(field, this->buf, this->size_bytes, this->size_bytes));
            memcpy(field, this->buf, this->size_bytes);
        }
    }
    if(file_header)
    {
        file_header->offset_data = offset_data;
        file_header->file_size = offset_data + (uint32_t)this->size_bytes;
    }
    return bmp_buf_pos;
}


void imgview::reset(char *ibuf, size_t sz, size_t width_, size_t height_,
                    size_t num_channels_, imgview::data_type_e dt)
{
    C4_CHECK(dt == data_u8 || dt == data_i8 || dt == data_u32 || dt == data_i32 || dt == data_f32);
    buf = ibuf;
    buf_size = sz;
    width = width_;
    height = height_;
    pixel_area = width * height;
    num_channels = num_channels_;
    num_bytes_per_channel = data_type_size(dt);
    size = pixel_area * num_channels;
    size_bytes = size * num_bytes_per_channel;
    data_type = dt;
    if(size_bytes > buf_size)
    {
        buf = nullptr;
        buf_size = 0;
    }
}


imgview load_bmp(char *buf, size_t sz)
{
    imgview v;
    v.load_bmp(buf, sz);
    return v;
}


imgview make_view(char *buf, size_t sz, size_t width, size_t height,
                  size_t num_channels, imgview::data_type_e dt)
{
    imgview v;
    v.reset(buf, sz, width, height, num_channels, dt);
    return v;
}

void vflip(imgview const& C4_RESTRICT src, imgview & C4_RESTRICT dst) noexcept
{
    C4_ASSERT(src.data_type == imgview::data_u8);
    C4_ASSERT(dst.data_type == imgview::data_u8);
    C4_ASSERT(src.buf != dst.buf);
    C4_ASSERT(src.width == dst.width);
    C4_ASSERT(src.height == dst.height);
    C4_ASSERT(src.pixel_area == dst.pixel_area);
    C4_ASSERT(src.num_channels == dst.num_channels);
    C4_ASSERT(!c4::mem_overlaps(src.buf, dst.buf, src.size_bytes, dst.size_bytes));
    using T = int8_t;
    using I = int32_t; // using signed indices for faster iteration
    const I H = (I)src.height;
    const I N = (I)src.width * (I)src.num_channels * (I)src.data_type_size();
    const size_t sN = (size_t)N;
    T const* C4_RESTRICT src_buf = (T const*) src.buf;
    T      * C4_RESTRICT dst_buf = (T      *) dst.buf;
    for(I h = 0; h < H; ++h)
    {
        T const* C4_RESTRICT src_row = src_buf +          h  * N;
        T      * C4_RESTRICT dst_row = dst_buf + (H - 1 - h) * N;
        memcpy(dst_row, src_row, sN);
    }
}

void convert_channels(imgview const& C4_RESTRICT src, imgview & C4_RESTRICT dst) noexcept
{
    C4_ASSERT(src.data_type == imgview::data_u8);
    C4_ASSERT(dst.data_type == imgview::data_u8);
    C4_ASSERT(src.buf != dst.buf);
    C4_ASSERT(src.width == dst.width);
    C4_ASSERT(src.height == dst.height);
    C4_ASSERT(src.pixel_area == dst.pixel_area);
    C4_ASSERT(src.buf_size / src.num_channels == dst.buf_size / dst.num_channels);
    C4_ASSERT(src.size_bytes / src.num_channels == dst.size_bytes / dst.num_channels);
    C4_ASSERT(!c4::mem_overlaps(src.buf, dst.buf, src.size_bytes, dst.size_bytes));
    using T = int8_t;
    using I = int32_t; // using signed indices for faster iteration
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

} // namespace quickgui

C4_SUPPRESS_WARNING_CLANG_POP
