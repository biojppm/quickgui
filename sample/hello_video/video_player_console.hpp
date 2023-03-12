#ifndef VIDEO_PLAYER_CONSOLE_H
#define VIDEO_PLAYER_CONSOLE_H

#include "video_reader.hpp"

namespace quickgui {

struct VideoPlayerConsole
{
    VideoReader m_reader;
    bool        m_reader_frame_ready;

    VideoPlayerConsole(quickgui::VideoSource const& src)
        : m_reader(src, false)
        , m_reader_frame_ready(false)
    {
    }

    bool finished() const { return m_reader.finished(); }

    void skip_frame()
    {
        C4_CHECK(m_reader.check_frame_ready());
        m_reader.read_frame();
    }

    std::chrono::nanoseconds time_curr() const
    {
        return m_reader.time_curr();
    }

    uint32_t frame_curr() const
    {
        return m_reader.frame_curr();
    }

    bool update(Frame *fr)
    {
        bool ready = m_reader.check_frame_ready();
        if(ready)
        {
            m_reader.read_frame();
            fr->index = frame_curr();
            fr->timestamp = time_curr();
            m_reader.vflip_frame();
            m_reader.convert_channels();
        }
        return ready;
    }
};


} // namespace quickgui

#endif /* VIDEO_PLAYER_CONSOLE_H */
