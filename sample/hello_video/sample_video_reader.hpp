#ifndef QUICKGUI_VIDEO_READER_H_
#define QUICKGUI_VIDEO_READER_H_

#include <quickgui/log.hpp>
#include <quickgui/imgview.hpp>
#include <quickgui/video/video_reader.hpp>

#include <vector>
#include <c4/format.hpp>
#include <c4/span.hpp>
using charspan = c4::span<char>;
#include <c4/fs/fs.hpp>

C4_SUPPRESS_WARNING_GCC_CLANG_PUSH
C4_SUPPRESS_WARNING_GCC_CLANG("-Wold-style-cast")

/// an owning image buffer
struct imgviewbuf
{
    using buffer_type = std::remove_const_t<typename quickgui::imgview::buffer_type>;
    std::vector<buffer_type> m_buf;
    quickgui::wimgview m_view;

    imgviewbuf() = default;
    bool empty() const { return m_buf.empty(); }
    void reset(quickgui::wimgview const& view)
    {
        m_buf.resize(view.bytes_required());
        m_view = view;
        m_view.buf = m_buf.data();
        m_view.buf_size = (uint32_t)m_buf.size();
        static_assert(sizeof(m_view.buf[0]) == sizeof(m_buf[0]));
    }
};

inline void convert(imgviewbuf const& src, imgviewbuf &dst)
{
    convert_channels(src.m_view, dst.m_view);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct SampleVideoReader
{
    quickgui::VideoReader m_reader;
    imgviewbuf        m_vframe_1ch_video;
    imgviewbuf        m_vframe_1ch_vflip;
    imgviewbuf        m_vframe_3ch_video;
    imgviewbuf        m_vframe_3ch_vflip;
    imgviewbuf        m_vframe_4ch_video;
    imgviewbuf        m_vframe_4ch_vflip;
    imgviewbuf       *m_vframe_video; // the original video frame from the reader
    imgviewbuf       *m_vframe_vflip; // the flipped frame from the original video
    std::vector<char> m_vframe_flip_workspace;

    SampleVideoReader(quickgui::VideoSource const& src, bool force_4ch=false)
        : m_reader(src)
        , m_vframe_1ch_video()
        , m_vframe_1ch_vflip()
        , m_vframe_3ch_video()
        , m_vframe_3ch_vflip()
        , m_vframe_4ch_video()
        , m_vframe_4ch_vflip()
        , m_vframe_video()
        , m_vframe_vflip()
    {
        QUICKGUI_LOGF("videoreader: datatype={} nchannels: reader={}", m_reader.data_type(), m_reader.num_channels());
        if(m_reader.num_channels() == 1)
        {
            // incoming video frame is single channel.
            // have to create 4 channels frame for rendering
            m_vframe_video = &m_vframe_1ch_video;
            m_vframe_vflip = &m_vframe_1ch_vflip;
            m_vframe_1ch_video.reset(m_reader.make_view(1));
            m_vframe_1ch_vflip.reset(m_reader.make_view(1));
            if(force_4ch)
            {
                QUICKGUI_LOGF("videoreader: 1ch -> 4ch");
                m_vframe_4ch_vflip.reset(m_reader.make_view(4));
            }
        }
        else if(m_reader.num_channels() == 3)
        {
            // incoming video frame is 3 channels.
            // have to create 1 channels frame for processing
            // have to create 4 channels frame for rendering
            QUICKGUI_LOGF("videoreader: 3ch -> 1ch");
            m_vframe_video = &m_vframe_3ch_video;
            m_vframe_vflip = &m_vframe_3ch_vflip;
            m_vframe_1ch_vflip.reset(m_reader.make_view(1));
            m_vframe_3ch_vflip.reset(m_reader.make_view(3));
            m_vframe_3ch_video.reset(m_reader.make_view(3));
            if(force_4ch)
            {
                QUICKGUI_LOGF("videoreader: 3ch -> 4ch");
                m_vframe_4ch_vflip.reset(m_reader.make_view(4));
            }
        }
        else if(m_reader.num_channels() == 4)
        {
            // incoming video frame is 4 channels.
            // have to create 1 channel frame for processing
            QUICKGUI_LOGF("videoreader: 4ch -> 1ch");
            m_vframe_video = &m_vframe_4ch_video;
            m_vframe_vflip = &m_vframe_4ch_vflip;
            m_vframe_1ch_vflip.reset(m_reader.make_view(1));
            m_vframe_4ch_video.reset(m_reader.make_view(4));
            m_vframe_4ch_vflip.reset(m_reader.make_view(4));
        }
    }

    uint32_t width() const noexcept { return m_reader.width(); }
    uint32_t height() const noexcept { return m_reader.height(); }
    quickgui::imgview::data_type_e data_type() const noexcept { return m_reader.data_type(); }

    uint32_t frame_curr() const noexcept { return m_reader.frame(); }
    std::chrono::nanoseconds time_curr() const noexcept { return m_reader.time(); }

    uint32_t num_frames() noexcept { return m_reader.num_frames(); }
    void frame_curr(uint32_t index) noexcept { m_reader.frame(index); }
    void time_curr(std::chrono::nanoseconds t) noexcept { m_reader.time(t); }

    bool check_frame_ready() { return m_reader.frame_grab(); }
    bool finished() const { return m_reader.finished(); }
    void read_frame() { C4_CHECK(m_reader.frame_read(&m_vframe_video->m_view)); }
    void vflip_frame() { vflip(m_vframe_video->m_view, m_vframe_vflip->m_view); }

    void copy_to_display_buffer(charspan mem)
    {
        C4_ASSERT(mem.size() >= m_vframe_4ch_vflip.m_view.bytes_required());
        memcpy(mem.data(), m_vframe_4ch_vflip.m_view.buf, m_vframe_4ch_vflip.m_view.bytes_required());
    }

    void convert_channels()
    {
        if(m_reader.num_channels() == 1)
        {
            if(m_vframe_3ch_vflip.m_view.bytes_required())
                convert(m_vframe_1ch_vflip, m_vframe_3ch_vflip);
            if(m_vframe_4ch_vflip.m_view.bytes_required())
                convert(m_vframe_1ch_vflip, m_vframe_4ch_vflip);
        }
        else if(m_reader.num_channels() == 3)
        {
            if(m_vframe_1ch_vflip.m_view.bytes_required())
                convert(m_vframe_3ch_vflip, m_vframe_1ch_vflip);
            if(m_vframe_4ch_vflip.m_view.bytes_required())
                convert(m_vframe_3ch_vflip, m_vframe_4ch_vflip);
        }
        else if(m_reader.num_channels() == 4)
        {
            if(m_vframe_3ch_vflip.m_view.bytes_required())
                convert(m_vframe_4ch_vflip, m_vframe_3ch_vflip);
            if(m_vframe_1ch_vflip.m_view.bytes_required())
                convert(m_vframe_4ch_vflip, m_vframe_1ch_vflip);
        }
    }

};

C4_SUPPRESS_WARNING_GCC_CLANG_POP

#endif /* QUICKGUI_VIDEO_READER_H_ */
