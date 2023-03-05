#ifndef QUICKGUI_STB_IMAGE_DATA_HPP_
#define QUICKGUI_STB_IMAGE_DATA_HPP_

#include <cstdint>
#include <cstddef>

#include "quickgui/string.hpp"

namespace quickgui {


enum class RequiredChannels : int {
    defer = 0, //< defer to the number of channels in the image
    one,       //< force 1 channel
    two,       //< force 2 channels
    three,     //< force 3 channels
    four,      //< force 4 channels
};


struct stb_image_data
{
    char *data;
    uint32_t width, height, num_channels;

    stb_image_data(quickgui::ccharspan file_contents, RequiredChannels channels=RequiredChannels::defer);
    ~stb_image_data();

    size_t data_size() const
    {
        constexpr const uint32_t num_bytes_per_channel = 1;
        return num_bytes_per_channel * num_channels * width * height;
    }

    quickgui::ccharspan to_span() const
    {
        return {data, data_size()};
    }
};


} // namespace quickgui

#endif /* QUICKGUI_STB_IMAGE_DATA_HPP_ */
