
#include "quickgui/video/video_reader.hpp"
#include "quickgui/video/video_source.hpp"
#include "quickgui/time.hpp"
#include <c4/std/string.hpp>
#include "quickgui/log.hpp"

#include <c4/fs/fs.hpp>
#include <c4/span.hpp>

#ifdef QUICKGUI_USE_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/log.h>
#include <libavdevice/avdevice.h>
} // extern "C"
#elif defined(QUICKGUI_USE_CV)
C4_SUPPRESS_WARNING_GCC_CLANG_PUSH
C4_SUPPRESS_WARNING_GCC_CLANG("-Wsign-conversion")
C4_SUPPRESS_WARNING_GCC_CLANG("-Wconversion")
C4_SUPPRESS_WARNING_GCC("-Wuseless-cast")
C4_SUPPRESS_WARNING_GCC("-Wfloat-equal")
#include <quickgui/cvutil/cvutil.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/videoio/videoio_c.h>
C4_SUPPRESS_WARNING_GCC_CLANG_POP
#endif


C4_SUPPRESS_WARNING_GCC_CLANG_PUSH
C4_SUPPRESS_WARNING_GCC_CLANG("-Wold-style-cast")

namespace quickgui {


VideoReader::~VideoReader() = default; // needed for the pimpl idiom with unique_ptr<Impl>

struct VideoReader::Impl
{
    VideoSource m_src;
    uint32_t    m_width;
    uint32_t    m_height;
    uint32_t    m_nframes;
    float       m_fps;
    fmsecs      m_dt;

    using FileEntries  = c4::fs::EntryList;
    using FileEntry    = c4::fs::EntryList::const_iterator;
    std::vector<char>  m_frame_data;
    imgview            m_frame_firstview;
    FileEntries        m_frame_filenames;
    FileEntry          m_frame_filenames_curr;
    FileEntry          m_frame_filenames_next;
    std::vector<char*> m_frame_filenames_names;
    std::vector<char > m_frame_filenames_arena;
    std::vector<char > m_frame_filenames_scratch;

    #ifdef QUICKGUI_USE_FFMPEG
    // https://ffmpeg.org/doxygen/trunk/decode_video_8c-example.html
    #define INBUF_SIZE 4096
    using AVPixelFormat_e = enum AVPixelFormat;
    using data_type_e = imgview::data_type_e;
    AVFormatContext      *m_avctx;
    AVStream             *m_avstream;
    unsigned              m_avistream;
    AVCodec const        *m_avcodec;
    AVCodecContext       *m_avcodec_ctx;
    AVCodecParserContext *m_avparser_ctx;
    AVFrame              *m_avframe;
    AVPacket             *m_avpacket;
    AVPixelFormat_e       m_avformat;
    data_type_e           m_avdata_type;
    uint32_t              m_avnum_channels;
    uint32_t              m_avbpp;
    FILE                 *m_avfile;
    uint8_t               m_avbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    #elif defined(QUICKGUI_USE_CV)
    cv::VideoCapture      m_cvcap;
    int                   m_cvformat;
    #endif

    ~Impl()
    {
    #ifdef QUICKGUI_USE_FFMPEG
        av_packet_free(&m_avpacket);
        av_frame_free(&m_avframe);
        avcodec_free_context(&m_avcodec_ctx);
        avformat_close_input(&m_avctx);
    #endif
    }

    Impl(VideoSource const& src)
        : m_src(src)
        , m_width()
        , m_height()
        , m_nframes()
        , m_fps()
        , m_dt()
        , m_frame_filenames()
        , m_frame_filenames_curr()
        , m_frame_filenames_next()
        , m_frame_filenames_names()
        , m_frame_filenames_arena()
        , m_frame_filenames_scratch()
    #ifdef QUICKGUI_USE_FFMPEG
        , m_avctx()
        , m_avstream()
        , m_avistream()
        , m_avcodec()
        , m_avcodec_ctx()
        , m_avframe()
        , m_avpacket()
        , m_avformat()
        , m_avdata_type()
        , m_avnum_channels()
        , m_avbpp()
        , m_avfile()
        , m_avbuf()
    #elif defined(QUICKGUI_USE_CV)
        , m_cvcap()
        , m_cvformat()
    #endif
    {
        #ifdef QUICKGUI_USE_CV
        auto getprops = [&]{
            m_cvformat = (int)m_cvcap.get(cv::CAP_PROP_FORMAT);
            m_width = (uint32_t)m_cvcap.get(cv::CAP_PROP_FRAME_WIDTH);
            m_height = (uint32_t)m_cvcap.get(cv::CAP_PROP_FRAME_HEIGHT);
            m_nframes = (m_src.source_type == VideoSource::FILE) ?
                (uint32_t)m_cvcap.get(cv::CAP_PROP_FRAME_COUNT) : 0u;
            m_fps = (float)m_cvcap.get(cv::CAP_PROP_FPS);
            m_cvcap.set(cv::CAP_PROP_FPS, 30.0);
            m_fps = (float)m_cvcap.get(cv::CAP_PROP_FPS);
            m_dt = fmsecs(1000.f / m_fps);
            cv::String backend = m_cvcap.getBackendName();
            int cvtype_orig = m_cvformat;
            m_cvformat = cvtype_to_video(m_cvformat);
            QUICKGUI_LOGF(R"(video properties:
    width={}
    height={}
    format={} --> {}
    #channels={}
    #frames={}
    fps={}/s
    dt={}ms
    backend={}
)",
                          m_width, m_height,
                          cvtype_str(cvtype_orig), cvtype_str(m_cvformat),
                          cvtype_lookup(m_cvformat).num_channels, m_nframes, m_fps, m_dt.count(),
                          c4::substr(backend.data(), backend.size()));
        };
        #endif
        switch(src.source_type)
        {
        case VideoSource::IMAGES:
            load_filenames(src.images.directory);
            break;
        case VideoSource::FILE:
        {
            #ifdef QUICKGUI_USE_FFMPEG
            // https://github.com/leandromoreira/ffmpeg-libav-tutorial/blob/master/0_hello_world.c
            C4_CHECK_MSG(c4::fs::file_exists(src.file.filename.c_str()), "video file does not exist: %s", src.file.filename.c_str());
            C4_CHECK(0 == avformat_open_input(&m_avctx, src.file.filename.c_str(), nullptr, nullptr));
            C4_CHECK(0 >= avformat_find_stream_info(m_avctx, nullptr));
            for(unsigned i = 0; i < m_avctx->nb_streams; ++i)
            {
                if(m_avctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
                {
                    m_avstream = m_avctx->streams[i];
                    m_avistream = i;
                    break;
                }
            }
            C4_CHECK(m_avstream != nullptr);
            QUICKGUI_LOGF("video stream={}/{} codec={} fmt={}", m_avistream, m_avctx->nb_streams, m_avstream->codecpar->codec_id, c4::to_csubstr(av_get_pix_fmt_name(m_avformat)));
            m_avcodec = avcodec_find_decoder(m_avstream->codecpar->codec_id);
            C4_CHECK(m_avcodec != nullptr);
            m_avparser_ctx = av_parser_init(m_avstream->codecpar->codec_id); // raw video 65.avi fails here
            C4_CHECK(m_avparser_ctx != nullptr);
            m_avcodec_ctx = avcodec_alloc_context3(m_avcodec);
            C4_CHECK(m_avcodec_ctx != nullptr);
            C4_CHECK(avcodec_parameters_to_context(m_avcodec_ctx, m_avstream->codecpar) >= 0);
            C4_CHECK(avcodec_open2(m_avcodec_ctx, m_avcodec, nullptr) >= 0);
            m_avframe = av_frame_alloc();
            m_avpacket = av_packet_alloc();
            C4_CHECK(m_avframe != nullptr);
            C4_CHECK(m_avpacket != nullptr);
            m_avformat = (AVPixelFormat_e)m_avstream->codecpar->format;
            AVPixFmtDescriptor const* fmtdesc = av_pix_fmt_desc_get(m_avformat);
            m_avnum_channels = (uint32_t)fmtdesc->nb_components;
            m_avbpp = (uint32_t)av_get_bits_per_pixel(fmtdesc);
            m_avdata_type = imgviewtype::u8;
            C4_CHECK(m_avbpp == 8 * fmtdesc->nb_components);
            m_width = (uint32_t)m_avstream->codecpar->width;
            m_height = (uint32_t)m_avstream->codecpar->height;
            m_nframes = (uint32_t)m_avstream->nb_frames;
            m_fps = (float)m_avstream->avg_frame_rate.num / (float)m_avstream->avg_frame_rate.den;
            m_dt = fmsecs(1000.f / m_fps);
            m_avfile = fopen(src.file.filename.c_str(), "rb");
            C4_CHECK(m_avfile != nullptr);
            QUICKGUI_LOGF(R"(opened video: {}
    stream={}/{}
    format={} [{}]
    #channels={}
    bits/px={}
    width={}
    height={}
    #frames={}
    fps={}/s
    dt={}ms
    duration={}s
    bitrate={}
)",
                     src.file.filename, m_avistream, m_avctx->nb_streams,
                     c4::to_csubstr(av_get_pix_fmt_name(m_avformat)), m_avctx->iformat->name,
                     m_avnum_channels, m_avbpp,
                     m_width, m_height, m_nframes, m_fps, m_dt.count(),
                     (float)m_avctx->duration/1.e6f, m_avctx->bit_rate
                     );
            #elif defined(QUICKGUI_USE_CV)
            C4_CHECK_MSG(c4::fs::file_exists(src.file.filename.c_str()), "video file does not exist: %s", src.file.filename.c_str());
            m_cvcap.open(cv::String(src.file.filename));
            C4_CHECK_MSG(m_cvcap.isOpened(), "failed to open video file: %s", src.file.filename.c_str());
            QUICKGUI_LOGF(R"(opened video: {})", src.file.filename);
            getprops();
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        }
        case VideoSource::CAMERA:
        {
            #ifdef QUICKGUI_USE_FFMPEG
            // https://ffmpeg.org/ffmpeg-all.html#video4linux2_002c-v4l2
            av_log_set_level(src.camera.log_level);
            avdevice_register_all(); // for device
            const AVInputFormat *input_fmt = av_find_input_format(src.camera.input_format);
            AVDictionary *options = NULL;
            C4_CHECK(av_dict_set(&options, "framerate", "20", 0) >= 0);
            AVFormatContext *pAVFormatContext = nullptr;
            // check video source
            C4_CHECK(avformat_open_input(&pAVFormatContext, src.camera.device, input_fmt, NULL) >= 0);
            #elif defined(QUICKGUI_USE_CV)
            m_cvcap.open(src.camera.index);
            C4_CHECK_MSG(m_cvcap.isOpened(), "failed to open camera[%d]", src.camera.index);
            QUICKGUI_LOGF(R"(opened camera: {})", src.camera.index);
            getprops();
            // https://stackoverflow.com/questions/16092802/capturing-1080p-at-30fps-from-logitech-c920-with-opencv-2-4-3
            bool src_overrides = false;
            if(src.camera.codec[0])
            {
                src_overrides = true;
                c4::csubstr c = {src.camera.codec, 4};
                QUICKGUI_LOGF("setting camera codec: {}", c)
                //C4_CHECK(m_cvcap.set(CV_CAP_PROP_FOURCC, CV_FOURCC('M', 'J', 'P', 'G')));
                m_cvcap.set(CV_CAP_PROP_FOURCC, CV_FOURCC(c[0], c[1], c[2], c[3]));
            }
            if(src.camera.width)
            {
                src_overrides = true;
                QUICKGUI_LOGF("setting camera width: {}", src.camera.width);
                //C4_CHECK(m_cvcap.set(CV_CAP_PROP_FRAME_WIDTH, src.camera.width));
                m_cvcap.set(CV_CAP_PROP_FRAME_WIDTH, src.camera.width);
            }
            if(src.camera.height)
            {
                src_overrides = true;
                QUICKGUI_LOGF("setting camera height: {}", src.camera.height);
                //C4_CHECK(m_cvcap.set(CV_CAP_PROP_FRAME_HEIGHT, src.camera.height));
                m_cvcap.set(CV_CAP_PROP_FRAME_HEIGHT, src.camera.height);
            }
            if(src_overrides)
            {
                getprops();
            }
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        }
        default:
            C4_NOT_IMPLEMENTED();
        }
    }

    void load_filenames(std::string const& directory)
    {
        C4_CHECK_MSG(c4::fs::dir_exists(directory.c_str()), "directory=%s", directory.c_str());
        if(m_frame_filenames_names.size() < 1024)
            m_frame_filenames_names.resize(1024);
        if(m_frame_filenames_arena.size() < 256)
            m_frame_filenames_arena.resize(256);
        if(m_frame_filenames_scratch.size() < 256)
            m_frame_filenames_scratch.resize(256);
        m_frame_filenames = {
            m_frame_filenames_arena.data(), m_frame_filenames_arena.size(),
            m_frame_filenames_names.data(), m_frame_filenames_names.size()};
        c4::fs::maybe_buf<char> scratch = {
            m_frame_filenames_scratch.data(), m_frame_filenames_scratch.size()};
        if(!c4::fs::list_entries(directory.c_str(), &m_frame_filenames, &scratch))
        {
            m_frame_filenames_arena.resize(m_frame_filenames.arena.required_size);
            m_frame_filenames_names.resize(m_frame_filenames.names.required_size);
            m_frame_filenames_scratch.resize(scratch.required_size);
            m_frame_filenames = {
                m_frame_filenames_arena.data(), m_frame_filenames_arena.size(),
                m_frame_filenames_names.data(), m_frame_filenames_names.size()};
            scratch = {m_frame_filenames_scratch.data(), m_frame_filenames_scratch.size()};
            C4_CHECK(c4::fs::list_entries(directory.c_str(), &m_frame_filenames, &scratch));
        }
        m_frame_filenames.sort();
        filter_filenames_for_bmp();
        m_nframes = 0;
        for(const char *filename : m_frame_filenames)
        {
            C4_UNUSED(filename);
            //QUICKGUI_LOGF("found {}", c4::to_csubstr(filename));
            C4_ASSERT(c4::fs::file_exists(filename));
            m_nframes++;
        }
        QUICKGUI_LOGF("found {} frames in {}", m_nframes, c4::to_csubstr(directory.c_str()));
        reset_frames();
        c4::fs::file_get_contents(*m_frame_filenames_next, &m_frame_data);
        m_frame_firstview = load_bmp(m_frame_data.data(), (uint32_t)m_frame_data.size());
        m_width = (uint32_t)m_frame_firstview.width;
        m_height = (uint32_t)m_frame_firstview.height;
        QUICKGUI_LOGF("video frames: {}x{}px #ch={} data_size={}B", m_width, m_height, m_frame_firstview.num_channels, m_frame_firstview.bytes_required());
    }

    void filter_filenames_for_bmp()
    {
        auto &ffn = m_frame_filenames;
        C4_CHECK(ffn.valid());
        size_t wpos = 0;
        for(size_t rpos = 0; rpos < ffn.names.required_size; ++rpos)
        {
            ffn.names.buf[wpos] = ffn.names.buf[rpos];
            wpos += is_bmp(ffn.names.buf[rpos]);
        }
        ffn.names.required_size = wpos;
    }

    static bool is_bmp(const char *filename) noexcept
    {
        return c4::to_csubstr(filename).ends_with(".bmp");
    }

    bool from_frames() const
    {
        return !m_frame_data.empty();
    }

    bool next_frame()
    {
        if(m_frame_filenames_next == ((c4::fs::EntryList const&)m_frame_filenames).end())
            return false;
        m_frame_filenames_curr = m_frame_filenames_next;
        ++m_frame_filenames_next;
        return m_frame_filenames_next != ((c4::fs::EntryList const&)m_frame_filenames).end();
    }

    void load_curr_frame(wimgview *v)
    {
        switch(m_src.source_type)
        {
        case VideoSource::CAMERA:
        case VideoSource::FILE:
        {
            #ifdef QUICKGUI_USE_CV
            cv::Size matsize(/*cols*/(int)m_width, /*rows*/(int)m_height);
            cv::Mat matview(matsize, m_cvformat, v->buf);
            // ensure that the cvmat is pointing at our data
            auto check_matview = [&]{
                if(!same_mat(matview, *v))
                {
                    //QUICKGUI_LOGF("elmsize={} #elms={} view={}({}) pimpl={}({})",
                    //         matview.elemSize(), matview.total(),
                    //         cvtype_str(matview.type()), matview.type(),
                    //         cvtype_str(m_cvformat), m_cvformat);
                    C4_ERROR("not same mat");
                }
            };
            check_matview();
            m_cvcap.retrieve(matview);
            check_matview();
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        }
        case VideoSource::IMAGES:
        {
            C4_ASSERT(m_frame_filenames_curr != ((c4::fs::EntryList const&)m_frame_filenames).end());
            c4::fs::file_get_contents(*m_frame_filenames_curr, &m_frame_data);
            *v = load_bmp(m_frame_data.data(), (uint32_t)m_frame_data.size());
            C4_CHECK(v->width == m_width);
            C4_CHECK(v->height == m_height);
            C4_CHECK(v->num_channels == m_frame_firstview.num_channels);
            break;
        }
        default:
            C4_NOT_IMPLEMENTED();
        }
    }

    void reset_frames(uint32_t frame_index=0)
    {
        m_frame_filenames_next = ((c4::fs::EntryList const&)m_frame_filenames).begin() + frame_index + 1;
        m_frame_filenames_curr = m_frame_filenames_next - 1;
    }

    void frame(uint32_t frame_index)
    {
        switch(m_src.source_type)
        {
        case VideoSource::IMAGES:
            reset_frames(frame_index);
            break;
        case VideoSource::CAMERA:
            C4_NOT_IMPLEMENTED();
            break;
        case VideoSource::FILE:
            #ifdef QUICKGUI_USE_OPENCV
            m_cvcap.set(cv::CAP_PROP_POS_FRAMES, (double)frame_index);
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        default:
            C4_NOT_IMPLEMENTED();
            break;
        }
    }

    uint32_t frame() const
    {
        switch(m_src.source_type)
        {
        case VideoSource::CAMERA:
            break;
        case VideoSource::IMAGES:
            C4_ASSERT(m_frame_filenames_curr >= m_frame_filenames.begin());
            return (uint32_t)(m_frame_filenames_curr - m_frame_filenames.begin());
        case VideoSource::FILE:
            #ifdef QUICKGUI_USE_OPENCV
            return (uint32_t)m_cvcap.get(cv::CAP_PROP_POS_FRAMES);
            #else
            C4_NOT_IMPLEMENTED();
            break;
            #endif
        default:
            C4_NOT_IMPLEMENTED();
            break;
        }
        return 0u;
    }

    void time(std::chrono::nanoseconds t)
    {
        switch(m_src.source_type)
        {
        case VideoSource::IMAGES:
            frame((uint32_t)((0.000001 + (double)m_src.images.fps * quickgui::dsecs(t).count())));
            break;
        case VideoSource::CAMERA:
            break;
        case VideoSource::FILE:
            #ifdef QUICKGUI_USE_OPENCV
            m_cvcap.set(cv::CAP_PROP_POS_MSEC, quickgui::dmsecs(t).count());
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        default:
            C4_NOT_IMPLEMENTED();
            break;
        }
    }

    std::chrono::nanoseconds time() const
    {
        using T = std::chrono::nanoseconds::rep;
        switch(m_src.source_type)
        {
        case VideoSource::IMAGES:
        {
            const double frame_index = (double)(m_frame_filenames_curr - m_frame_filenames.begin());
            const double secs_sofar = frame_index / (double)m_src.images.fps;
            return std::chrono::nanoseconds((T)(1.e9 * secs_sofar));
        }
        case VideoSource::CAMERA:
            break;
        case VideoSource::FILE:
            #ifdef QUICKGUI_USE_OPENCV
            return std::chrono::nanoseconds((T)(1.e6 * m_cvcap.get(cv::CAP_PROP_POS_MSEC)));
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        default:
            C4_NOT_IMPLEMENTED();
        }
        return std::chrono::nanoseconds::zero();
    }

    bool finished() const
    {
        switch(m_src.source_type)
        {
        case VideoSource::IMAGES:
            if(m_src.images.loop)
                return false;
            return m_frame_filenames_next == m_frame_filenames.end();
        case VideoSource::CAMERA:
            return false;
        case VideoSource::FILE:
            #ifdef QUICKGUI_USE_OPENCV
            if(m_src.file.loop)
                return false;
            return (uint32_t)m_cvcap.get(cv::CAP_PROP_POS_FRAMES) == (uint32_t)m_cvcap.get(cv::CAP_PROP_FRAME_COUNT);
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        default:
            C4_NOT_IMPLEMENTED();
        }
        return false;
    }

};


//-----------------------------------------------------------------------------

VideoReader::VideoReader(VideoSource const& src)
    : m_pimpl()
    , m_curr_loop()
    , m_loop(src.loop())
{
    m_pimpl = std::make_unique<VideoReader::Impl>(src);
}

uint32_t VideoReader::width() const
{
    return m_pimpl->m_width;
}

uint32_t VideoReader::height() const
{
    return m_pimpl->m_height;
}

size_t VideoReader::frame_bytes() const
{
    if(m_pimpl->from_frames())
        return m_pimpl->m_frame_firstview.bytes_required();
#ifdef QUICKGUI_USE_FFMPEG
    C4_ASSERT((m_pimpl->m_avbpp & 7) == 0);
    return m_pimpl->m_avbpp / 8 * area();
#elif defined(QUICKGUI_USE_CV)
    return cvtype_bytes(m_pimpl->m_cvformat) * area();
#else
    C4_NOT_IMPLEMENTED();
    return 0;
#endif
}

imgview::data_type_e VideoReader::data_type() const
{
    if(m_pimpl->from_frames())
        return m_pimpl->m_frame_firstview.data_type;
#ifdef QUICKGUI_USE_FFMPEG
    return m_pimpl->m_avdata_type;
#elif defined(QUICKGUI_USE_CV)
    return cvtype_lookup(m_pimpl->m_cvformat).data_type;
#else
    C4_NOT_IMPLEMENTED();
    return {};
#endif
}

uint32_t VideoReader::num_channels() const
{
    if(m_pimpl->from_frames())
        return (uint32_t)m_pimpl->m_frame_firstview.num_channels;
#ifdef QUICKGUI_USE_FFMPEG
    return m_pimpl->m_avnum_channels;
#elif defined(QUICKGUI_USE_CV)
    return cvtype_lookup(m_pimpl->m_cvformat).num_channels;
#else
    C4_NOT_IMPLEMENTED();
    return {};
#endif
}

bool VideoReader::finished() const
{
    return m_pimpl->finished();
}

uint32_t VideoReader::num_frames() const
{
    return m_pimpl->m_nframes;
}

float VideoReader::video_fps() const
{
    return m_pimpl->m_fps;
}

wimgview VideoReader::make_view(uint32_t force_numchannels) const
{
    uint32_t ch = force_numchannels ? force_numchannels : num_channels();
    return quickgui::make_wimgview(nullptr, 0, width(), height(), ch, data_type());
}

bool VideoReader::frame_grab()
{
    auto reloop = [this]{
        ++m_curr_loop;
        QUICKGUI_LOGF("looping video: #frames={} #loops={}x", m_pimpl->m_nframes, m_curr_loop);
    };
    if(m_pimpl->from_frames())
    {
        if(m_pimpl->next_frame())
        {
            return true;
        }
        else
        {
            if(m_loop)
            {
                reloop();
                m_pimpl->reset_frames();
                m_pimpl->next_frame();
                return true;
            }
        }
        return false;
    }
#ifdef QUICKGUI_USE_FFMPEG
    if(feof(m_pimpl->m_avfile))
    {
        if(m_loop)
        {
            reloop();
            C4_CHECK(fseek(m_pimpl->m_avfile, 0, SEEK_SET));
        }
        else
        {
            return false;
        }
    }
    // This is a WIP, taken from ffmpeg's decode sample:
    //
    // https://ffmpeg.org/doxygen/trunk/decode_video_8c-example.html
    //
    // The call to av_parser_parse2() is causing this message from
    // ffmpeg:
    //
    // [h264 @ 0x56180330ec80] AVC-parser: nal size 1718773093 remaining 4056
    // [h264 @ 0x56180330ec80] SEI type 116 size 968 truncated at 40
    // [h264 @ 0x56180330ec80] Invalid NAL unit size (1718773093 > 4056).
    // [h264 @ 0x56180330ec80] missing picture in access unit with size 4096
    //
    // ... and then the call to av_send_packet() is causing this:
    //
    // [h264 @ 0x56180330ec80] Invalid NAL unit size (1718773093 > 4056).
    // [h264 @ 0x56180330ec80] Error splitting the input into NAL units.
    //
    // ... where the return code < 0 and none of the documented return
    // codes in the function documentation.
    //
    // This may (but not likely) have to do with a bug:
    // https://superuser.com/questions/785343/what-does-the-ffmpeg-warning-missing-picture-in-access-unit-with-size-x-mean
    //
    // As a different approach, tried to read the RAW AVI video
    // YUV420p but then the av_parser_init() call to create the parser
    // context fails.
    //
    // So, for now, we will stick to reading frames from BMP images.
    //
    // see also:
    //
    // https://stackoverflow.com/questions/57527547/what-does-it-mean-invalid-nal-unit-size-for-h-264-decoder
    // https://stackoverflow.com/questions/46601724/h264-inside-avi-mp4-and-raw-h264-streams-different-format-of-nal-units-or-f?rq=1
    // https://stackoverflow.com/questions/24884827/possible-locations-for-sequence-picture-parameter-sets-for-h-264-stream
    bool has_frame = false;
    QUICKGUI_LOGF("aqui 0");
    do
    {
        size_t data_size = fread(m_pimpl->m_avbuf, 1, INBUF_SIZE, m_pimpl->m_avfile);
        C4_CHECK(data_size != 0);
        c4::span<uint8_t> buf = m_pimpl->m_avbuf;
        QUICKGUI_LOGF("aqui 1 bufsz={} datasz={}", buf.size(), data_size);
        buf = buf.first(data_size);
        QUICKGUI_LOGF("aqui 1.1");
        while(buf.size())
        {
            QUICKGUI_LOGF("aqui 1.2: bufsz={}", buf.size());
            int ret = av_parser_parse2(m_pimpl->m_avparser_ctx, m_pimpl->m_avcodec_ctx,
                                       &m_pimpl->m_avpacket->data, &m_pimpl->m_avpacket->size,
                                       buf.data(), (int)buf.size(),
                                       AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            QUICKGUI_LOGF("aqui 1.3 ret={}", ret);
            C4_CHECK(ret >= 0);
            buf = buf.subspan((size_t)ret);
            if(m_pimpl->m_avpacket->size > 0)
            {
                QUICKGUI_LOGF("aqui 1.3.1");
                ret = avcodec_send_packet(m_pimpl->m_avcodec_ctx, m_pimpl->m_avpacket);
                switch(ret)
                {
                case AVERROR(EAGAIN):
                    QUICKGUI_LOGF("aqui 1.3.2 EAGAIN");
                    break;
                case AVERROR(EINVAL):
                    QUICKGUI_LOGF("aqui 1.3.2 EINVAL");
                    break;
                case AVERROR(ENOMEM):
                    QUICKGUI_LOGF("aqui 1.3.2 ENOMEM");
                    break;
                case AVERROR_EOF:
                    QUICKGUI_LOGF("aqui 1.3.2 EOF");
                    break;
                default:
                    QUICKGUI_LOGF("aqui 1.3.2 ok! ret={}", ret);
                    C4_CHECK(ret >= 0); // mp4-encoded fails here...
                    has_frame = true;
                    break;
                }
            }
            QUICKGUI_LOGF("aqui 1.4");
        }
    QUICKGUI_LOGF("aqui 1.5");
    } while(!has_frame);
    QUICKGUI_LOGF("aqui 2");
    C4_CHECK(has_frame);
    QUICKGUI_LOGF("aqui 3");
    C4_CHECK(avcodec_receive_frame(m_pimpl->m_avcodec_ctx, m_pimpl->m_avframe));
    QUICKGUI_LOGF("aqui 4");
    return has_frame;
#elif defined(QUICKGUI_USE_CV)
    bool gotit = m_pimpl->m_cvcap.grab();
    if(!gotit)
    {
        if(m_loop)
        {
            reloop();
            C4_CHECK(m_pimpl->m_cvcap.set(cv::CAP_PROP_POS_FRAMES, 0));
            gotit = m_pimpl->m_cvcap.grab();
        }
    }
    return gotit;
#else
    C4_NOT_IMPLEMENTED();
    return {};
#endif
}

bool VideoReader::frame_read(wimgview *v)
{
    C4_ASSERT(v);
    // exit early if the data cannot accomodate the frame
    if(v->bytes_required() < frame_bytes())
        return false;
    if(m_pimpl->from_frames())
    {
        m_pimpl->load_curr_frame(v);
        return true;
    }
#ifdef QUICKGUI_USE_FFMPEG
    AVFrame *f = m_pimpl->m_avframe;
    C4_ASSERT(f->width > 0);
    C4_ASSERT(f->height > 0);
    C4_ASSERT(f->width == (int)m_pimpl->m_width);
    C4_ASSERT(f->height == (int)m_pimpl->m_height);
    C4_NOT_IMPLEMENTED();
    for(int h = 0; h < f->height; ++h)
    {
        // WIP
    }
    return false;
#elif defined(QUICKGUI_USE_CV)
    m_pimpl->load_curr_frame(v);
    return true;
#else
    C4_NOT_IMPLEMENTED();
    return false;
#endif
}

uint32_t VideoReader::frame() const
{
    return m_pimpl->frame();
}

void VideoReader::frame(uint32_t frame_index)
{
    m_pimpl->frame(frame_index);
}

std::chrono::nanoseconds VideoReader::time() const
{
    return m_pimpl->time();
}

void VideoReader::time(std::chrono::nanoseconds t)
{
    m_pimpl->time(t);
}

} // namespace quickgui

C4_SUPPRESS_WARNING_GCC_CLANG_POP
