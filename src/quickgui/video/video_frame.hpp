#ifndef QUICKGUI_VIDEO_VIDEO_FRAME_HPP_
#define QUICKGUI_VIDEO_VIDEO_FRAME_HPP_

#include <chrono>
#include <quickgui/imgview.hpp>

namespace quickgui {

struct VideoFrame
{
    std::chrono::nanoseconds timestamp;
    uint32_t index;
    imgview img;
};

} // namespace quickgui

#endif /* QUICKGUI_VIDEO_VIDEO_FRAME_HPP_ */
