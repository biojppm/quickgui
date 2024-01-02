#ifndef QUICKGUI_VIDEO_VIDEO_READER_HPP_
#define QUICKGUI_VIDEO_VIDEO_READER_HPP_

#include <cstdint>
#include <memory>
#include <string>
#include <chrono>
#include "quickgui/imgview.hpp"
#include "quickgui/time.hpp"

namespace quickgui {

struct VideoSource;
struct VideoReader
{
    VideoReader(VideoSource const& src);
    ~VideoReader();

public: // video info

    uint32_t width() const;
    uint32_t height() const;
    uint32_t area() const { return width() * height(); }
    size_t   frame_bytes() const;
    imgview::data_type_e data_type() const;
    uint32_t num_channels() const;

    bool     finished() const;
    uint32_t num_frames() const;
    float    video_fps() const;

    wimgview  make_view(uint32_t force_numchannels=0) const;

public: // player actions

    /** grab a frame if it is ready */
    bool frame_grab();
    /** requires a frame to be ready (prior call to frame_grab() returning true)  */
    bool frame_read(wimgview *view);

    void frame(uint32_t frame);
    void time(std::chrono::nanoseconds time);

public: // player state

    uint32_t frame() const;
    std::chrono::nanoseconds time() const;
    uint32_t loop_curr() const;

public:

    struct Impl;
    std::unique_ptr<Impl> m_pimpl;
};

} // namespace quickgui

#endif /* QUICKGUI_VIDEO_VIDEO_READER_HPP_ */
