
#include "quickgui/video/video_reader.hpp"
#include "quickgui/video/video_source.hpp"
#include "quickgui/time.hpp"
#include <c4/std/string.hpp>
#include "quickgui/log.hpp"

#include <c4/fs/fs.hpp>
#include <c4/span.hpp>

// @todo use native libraries:
//    linux: v4l https://linuxtv.org/downloads/v4l-dvb-apis/
//    windows: dshow
#ifdef QUICKGUI_USE_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/mediacodec.h>
#include <libavformat/avformat.h>
#include <libavutil/log.h>
#include <libavutil/pixdesc.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
} // extern "C"
#include <c4/format.hpp>
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

#ifdef QUICKGUI_USE_FFMPEG
void setopt_str_(AVDictionary **dict, const char *prop_name, const char* val_str)
{
    QUICKGUI_LOGF("{}: [{}] {}={}", c4::fmt::hex(*dict), av_dict_count(*dict), prop_name, val_str);
    C4_CHECK(av_dict_set(dict, prop_name, val_str, 0) >= 0);
}
template<class T>
void setopt_(AVDictionary **dict, const char *prop_name, T val)
{
    char val_str[256];
    C4_CHECK(c4::cat(val_str, val, '\0') < sizeof(val_str));
    setopt_str_(dict, prop_name, val_str);
}
template<class ...Args>
void setopt_fmt_(AVDictionary **dict, const char *prop_name, c4::csubstr fmt, Args const & ...args)
{
    char val_str[256];
    C4_ASSERT(fmt.ends_with('\0'));
    c4::csubstr ret = c4::format_sub(val_str, fmt, args...);
    C4_CHECK(ret.len < sizeof(val_str));
    setopt_str_(dict, prop_name, ret.str);
}
void print_stream(const char *prefix, AVStream const* stream)
{
    C4_ASSERT(stream);
    AVCodecParameters const* codecpar = stream->codecpar;
    C4_ASSERT(codecpar);
    QUICKGUI_LOGF("{}codec_type {} ({})", prefix, av_get_media_type_string(codecpar->codec_type), codecpar->codec_type);
    QUICKGUI_LOGF("{}time_base before open coded {}/{}", prefix, stream->time_base.num, stream->time_base.den);
    QUICKGUI_LOGF("{}r_frame_rate before open coded {}/{}", prefix, stream->r_frame_rate.num, stream->r_frame_rate.den);
    QUICKGUI_LOGF("{}start_time {}", prefix, stream->start_time);
    QUICKGUI_LOGF("{}duration {}", prefix, stream->duration);
    if(codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        using FourCC = const char (&)[4];
        FourCC fourcc = (FourCC)codecpar->codec_tag;
        AVPixelFormat fmt = (AVPixelFormat)codecpar->format;
        AVPixFmtDescriptor const* desc = av_pix_fmt_desc_get(fmt);
        QUICKGUI_LOGF("{}video codec format {}", prefix, desc->name);
        QUICKGUI_LOGF("{}video codec fourcc {}.{}.{}.{}", prefix, fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
        QUICKGUI_LOGF("{}video codec resolution {}x{}", prefix, codecpar->width, codecpar->height);
        QUICKGUI_LOGF("{}video codec framerate {}/{}", prefix, stream->r_frame_rate.num, stream->r_frame_rate.den);
        QUICKGUI_LOGF("{}video codec bitrate {} bit/s", prefix, codecpar->bit_rate);
        QUICKGUI_LOGF("{}video codec bits/coded_sample {} bits", prefix, codecpar->bits_per_coded_sample);
        QUICKGUI_LOGF("{}video codec pixfmt num components {}", prefix, desc->nb_components);
        QUICKGUI_LOGF("{}video codec pixfmt bits per pixel {} bits", prefix, av_get_bits_per_pixel(desc));
    }
}

struct ReaderAVFile
{
    // https://ffmpeg.org/doxygen/trunk/decode_video_8c-example.html
    #define INBUF_SIZE 4096
    using AVPixelFormat_e = enum AVPixelFormat;
    using data_type_e = imgview::data_type_e;
    AVFormatContext      *m_avctx = {};
    AVStream             *m_avstream = {};
    unsigned              m_avistream = {};
    AVCodec const        *m_avcodec = {};
    AVCodecContext       *m_avcodec_ctx = {};
    AVCodecParserContext *m_avparser_ctx = {};
    AVFrame              *m_avframe = {};
    AVPacket             *m_avpacket = {};
    AVPixelFormat_e       m_avformat = {};
    data_type_e           m_avdata_type = {};
    uint32_t              m_avnum_channels = {};
    uint32_t              m_avbpp = {};
    FILE                 *m_avfile = {};
    uint8_t               m_avbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE] = {};
    int m_width = {};
    int m_height = {};
    uint32_t m_num_channels = {};
    uint32_t m_bits_per_pixel = {};
    imgviewtype::data_type_e m_data_type;
    uint32_t           m_nframes = {};
    float              m_fps = {};
    fmsecs             m_dt = {};
    VideoSource::VideoSourceFile m_src = {};
    uint32_t m_curr_loop = {};

    ~ReaderAVFile()
    {
        destroy();
    }

    void destroy()
    {
        av_packet_free(&m_avpacket);
        av_frame_free(&m_avframe);
        avcodec_free_context(&m_avcodec_ctx);
        avformat_close_input(&m_avctx);
    }
    void init(VideoSource::VideoSourceFile const& src)
    {
        m_src = src;
        // https://github.com/leandromoreira/ffmpeg-libav-tutorial/blob/master/0_hello_world.c
        C4_CHECK_MSG(c4::fs::file_exists(src.filename.c_str()), "video file does not exist: %s", src.filename.c_str());
        C4_CHECK(0 == avformat_open_input(&m_avctx, src.filename.c_str(), nullptr, nullptr));
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
        m_width = (int)m_avstream->codecpar->width;
        m_height = (int)m_avstream->codecpar->height;
        m_nframes = (uint32_t)m_avstream->nb_frames;
        m_fps = (float)m_avstream->avg_frame_rate.num / (float)m_avstream->avg_frame_rate.den;
        m_dt = fmsecs(1000.f / m_fps);
        m_avfile = fopen(src.filename.c_str(), "rb");
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
                      src.filename, m_avistream, m_avctx->nb_streams,
                      c4::to_csubstr(av_get_pix_fmt_name(m_avformat)), m_avctx->iformat->name,
                      m_avnum_channels, m_avbpp,
                      m_width, m_height, m_nframes, m_fps, m_dt.count(),
                      (float)m_avctx->duration/1.e6f, m_avctx->bit_rate
            );
    }

    size_t frame_bytes() const
    {
        C4_ASSERT((m_avbpp & 7u) == 0);
        return (size_t)(m_avbpp / 8u * ((uint32_t)m_width * (uint32_t)m_height));
    }

    imgviewtype::data_type_e data_type() const
    {
        return m_avdata_type;
    }

    uint32_t num_channels() const
    {
        return m_avnum_channels;
    }

    bool frame_grab()
    {
        auto reloop = [this]{
            ++m_curr_loop;
            QUICKGUI_LOGF("looping video: #frames={} #loops={}x", m_nframes, m_curr_loop);
        };
        if(feof(m_avfile))
        {
            if(m_src.loop)
            {
                reloop();
                C4_CHECK(fseek(m_avfile, 0, SEEK_SET));
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
            size_t data_size = fread(m_avbuf, 1, INBUF_SIZE, m_avfile);
            C4_CHECK(data_size != 0);
            c4::span<uint8_t> buf = m_avbuf;
            QUICKGUI_LOGF("aqui 1 bufsz={} datasz={}", buf.size(), data_size);
            buf = buf.first(data_size);
            QUICKGUI_LOGF("aqui 1.1");
            while(buf.size())
            {
                QUICKGUI_LOGF("aqui 1.2: bufsz={}", buf.size());
                int ret = av_parser_parse2(m_avparser_ctx, m_avcodec_ctx,
                                           &m_avpacket->data, &m_avpacket->size,
                                           buf.data(), (int)buf.size(),
                                           AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
                QUICKGUI_LOGF("aqui 1.3 ret={}", ret);
                C4_CHECK(ret >= 0);
                buf = buf.subspan((size_t)ret);
                if(m_avpacket->size > 0)
                {
                    QUICKGUI_LOGF("aqui 1.3.1");
                    ret = avcodec_send_packet(m_avcodec_ctx, m_avpacket);
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
        C4_CHECK(avcodec_receive_frame(m_avcodec_ctx, m_avframe));
        QUICKGUI_LOGF("aqui 4");
        return has_frame;
    }

    bool frame_read(wimgview *v)
    {
        C4_ASSERT(m_avframe->width > 0);
        C4_ASSERT(m_avframe->height > 0);
        C4_ASSERT(m_avframe->width == (int)m_width);
        C4_ASSERT(m_avframe->height == (int)m_height);
        C4_NOT_IMPLEMENTED(); (void)v;
        for(int h = 0; h < m_avframe->height; ++h)
        {
            // WIP
        }
        return false;
    }

    uint32_t frame() const
    {
        return 0u; // FIXME
    }

    void frame(uint32_t frame_index) const
    {
        (void)frame_index;
    }

    bool finished() const
    {
        return false; // FIXME
    }

    std::chrono::nanoseconds time() const
    {
        return {}; // FIXME
    }

    void time(std::chrono::nanoseconds t)
    {
        (void)t; // FIXME
    }

};


C4_NO_INLINE void averr__(int errcode, const char *file, int line, const char *stmt)
{
    char errmsg[128];
    av_make_error_string(errmsg, sizeof(errmsg), errcode);
    c4::handle_error(c4::srcloc{file, line}, "%s: (%d) %s", stmt, errcode, errmsg);
}


#define AVCHECK(stmt)                                   \
    do {                                                \
        int C4_XCAT(ret__, __LINE__) = (stmt);          \
        if(C4_UNLIKELY(C4_XCAT(ret__, __LINE__) < 0))   \
            averr__(C4_XCAT(ret__, __LINE__),           \
                    __FILE__, __LINE__, #stmt);         \
    } while(0)


struct ReaderAVCam
{
    AVFormatContext *m_fmt_ctx = nullptr;
    AVStream const* m_stream = nullptr;
    unsigned int m_stream_index = 0;
    AVCodec const* m_codec = nullptr;
    AVCodecContext *m_codec_ctx = nullptr;
    AVFrame *m_frame = nullptr; ///< https://ffmpeg.org/doxygen/trunk/structAVFrame.html
    AVPacket *m_packet = nullptr;
    int m_width;
    int m_height;
    float m_fps;
    fmsecs m_dt;
    AVPixelFormat m_avformat;
    uint32_t m_num_channels;
    uint32_t m_bits_per_pixel;
    imgviewtype::data_type_e m_data_type;

    ~ReaderAVCam()
    {
        destroy();
    }
    void destroy()
    {
        av_packet_free(&m_packet);
        av_frame_free(&m_frame);
        avcodec_free_context(&m_codec_ctx);
        avformat_close_input(&m_fmt_ctx);
        av_free(m_fmt_ctx);
        m_fmt_ctx = nullptr;
        memset(this, 0, sizeof(*this));
    }
    void init(VideoSource::VideoSourceCamera const& cam)
    {
        // https://github.com/leandromoreira/ffmpeg-libav-tutorial/blob/master/0_hello_world.c
        // https://ffmpeg.org/ffmpeg-all.html#video4linux2_002c-v4l2
        av_log_set_level(cam.log_level);
        // WTF? av_register_all();
        // WTF? avcodec_register_all();
        QUICKGUI_LOGF("register all");
        avdevice_register_all();
        const AVInputFormat *input_fmt = av_find_input_format(cam.input_device);
        C4_CHECK_MSG(input_fmt != nullptr, "failed to find input format %s", cam.input_device);
        QUICKGUI_LOGF("alloc_context");
        m_fmt_ctx = avformat_alloc_context();
        // set options
        //m_fmt_ctx->video_codec_id = AV_CODEC_ID_MJPEG;
        AVDictionary *options = nullptr;
        setopt_(&options, "input_format", cam.input_format);
        setopt_(&options, "framerate", cam.framerate);
        setopt_fmt_(&options, "video_size", "{}x{}\0", cam.width, cam.height);
        QUICKGUI_LOGF("device options: {}", av_dict_count(options));
        // check video source
        QUICKGUI_LOGF("opening device: {}", cam.device);
        AVCHECK(avformat_open_input(&m_fmt_ctx, cam.device, input_fmt, &options));
        QUICKGUI_LOGF("#ignored device options: {}", av_dict_count(options));
        AVDictionaryEntry *t = NULL;
        while ((t = av_dict_get(options, "", t, AV_DICT_IGNORE_SUFFIX)))
        {
            QUICKGUI_LOGF("ignoring device option: {}={}", t->key, t->value);
        }
        av_free(options);
        QUICKGUI_LOGF("dump format");
        av_dump_format(m_fmt_ctx, 0, cam.device, 0);
        QUICKGUI_LOGF("find stream info");
        AVCHECK(avformat_find_stream_info(m_fmt_ctx, nullptr));
        // search stream index
        m_codec = nullptr;
        m_stream = nullptr;
        m_stream_index = m_fmt_ctx->nb_streams + 1;
        for(unsigned int i = 0; i < m_fmt_ctx->nb_streams; ++i)
        {
            AVStream const* stream = m_fmt_ctx->streams[i];
            QUICKGUI_LOGF("stream[{}]:", i);
            print_stream("   ", stream);
            AVCodecParameters const* codecpar = stream->codecpar;
            if(codecpar->codec_type != AVMEDIA_TYPE_VIDEO)
            {
                QUICKGUI_LOGF("stream[{}]: codec is not video.");
                continue;
            }
            // find the registered decoder for a codec ID
            // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
            AVCodec const* local_codec = avcodec_find_decoder(codecpar->codec_id);
            if(local_codec == nullptr)
            {
                QUICKGUI_LOGF("stream[{}]: unsupported codec.");
                continue;
            }
            if(m_stream_index >= m_fmt_ctx->nb_streams)
            {
                QUICKGUI_LOGF("stream[{}]: codec is video. choosing.", i);
                m_stream = m_fmt_ctx->streams[i];
                m_stream_index = i;
                m_codec = local_codec;
                break;
            }
        }
        if(!m_stream)
        {
            C4_ERROR("found no stream with video codec");
        }
        C4_ASSERT(m_codec);
        C4_ASSERT(m_stream_index < m_fmt_ctx->nb_streams);
        AVCodecParameters *codecpar = m_stream->codecpar;
        // https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
        m_codec_ctx = avcodec_alloc_context3(m_codec);
        C4_CHECK(m_codec_ctx != nullptr);
        // Fill the codec context based on the values from the supplied codec parameters
        // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
        AVCHECK(avcodec_parameters_to_context(m_codec_ctx, codecpar));
        // Initialize the AVCodecContext to use the given AVCodec.
        // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d
        AVCHECK(avcodec_open2(m_codec_ctx, m_codec, NULL));
        // https://ffmpeg.org/doxygen/trunk/structAVFrame.html
        m_frame = av_frame_alloc();
        C4_CHECK(m_frame != nullptr);
        // https://ffmpeg.org/doxygen/trunk/structAVPacket.html
        m_packet = av_packet_alloc();
        C4_CHECK(m_packet != nullptr);
        m_width = codecpar->width;
        m_height = codecpar->height;
        m_fps = (float)m_stream->avg_frame_rate.num / (float)m_stream->avg_frame_rate.den;
        m_dt = fmsecs(1000.f / m_fps);
        m_avformat = (AVPixelFormat)m_stream->codecpar->format;
        AVPixFmtDescriptor const* fmtdesc = av_pix_fmt_desc_get(m_avformat);
        m_num_channels = (uint32_t)fmtdesc->nb_components;
        m_bits_per_pixel = (uint32_t)av_get_bits_per_pixel(fmtdesc);
    }

    size_t frame_bytes() const
    {
        return (size_t)(m_num_channels * (uint32_t)m_width * (uint32_t)m_height);  // FIXME
    }

    imgviewtype::data_type_e data_type() const
    {
        return imgviewtype::u8;
    }

    uint32_t num_channels() const
    {
        return m_num_channels;
    }

    bool m_packet_fresh = false;
    bool m_packet_finished = true;
    bool frame_grab()
    {
        if(!m_packet_finished)
            return true;
        QUICKGUI_LOGF("... read frame");
        const int ret = av_read_frame(m_fmt_ctx, m_packet);
        bool has_one = (ret >= 0) && ((unsigned int)m_packet->stream_index == m_stream_index);
        if(has_one)
        {
            m_packet_fresh = true;
            m_packet_finished = false;
        }
        return has_one;
    }

    bool frame_read(wimgview *v)
    {
        if(m_packet_fresh)
        {
            QUICKGUI_LOGF("... send packet");
            AVCHECK(avcodec_send_packet(m_codec_ctx, m_packet));
            av_packet_unref(m_packet);
            m_packet_fresh = false;
        }
        QUICKGUI_LOGF("... recv frame");
        int ret = avcodec_receive_frame(m_codec_ctx, m_frame);
        if(C4_UNLIKELY(ret < 0))
        {
            if(ret == AVERROR(EAGAIN))
            {
                QUICKGUI_LOGF("... EAGAIN");
                return true;
            }
            else if(ret == AVERROR_EOF)
            {
                QUICKGUI_LOGF("... EOF");
                return true;
            }
            else if(ret < 0)
            {
                averr__(ret, __FILE__, __LINE__, "avcodec_receive_frame");
                return false;
            }
        }
        m_packet_finished = true;
        C4_SUPPRESS_WARNING_GCC_CLANG_WITH_PUSH("-Wdeprecated-declarations");
        QUICKGUI_LOGF("Frame {} (type={}, size={}B, format={}({})) pts {} key_frame {} [DTS {}]",
                      m_codec_ctx->frame_number,
                      av_get_picture_type_char(m_frame->pict_type),
                      m_frame->pkt_size,
                      m_frame->format,
                      av_pix_fmt_desc_get((AVPixelFormat)m_frame->format) ? av_pix_fmt_desc_get((AVPixelFormat)m_frame->format)->name : "unknown",
                      m_frame->pts,
                      m_frame->key_frame,
                      m_frame->coded_picture_number);
        C4_SUPPRESS_WARNING_GCC_CLANG_POP
        //C4_ASSERT_MSG(m_frame->format == AV_PIX_FMT_YUV422P, "format was %s", av_pix_fmt_desc_get((AVPixelFormat)m_frame->format)->name);
        C4_ASSERT(m_frame->format == AV_PIX_FMT_YUYV422);
        C4_ASSERT(m_frame->crop_top == 0u);
        C4_ASSERT(m_frame->crop_bottom == 0u);
        C4_ASSERT(m_frame->crop_left == 0u);
        C4_ASSERT(m_frame->crop_right == 0u);
        yuy2view yuy2;
        yuy2.reset(m_frame->data[0], (uint32_t)m_frame->linesize[0], (uint32_t)m_frame->width, (uint32_t)m_frame->height, imgviewtype::u8);
        convert_yuy2_to_rgb(yuy2, *v);
        return true;
    }

    uint32_t frame() const
    {
        return 0u; // FIXME
    }

    void frame(uint32_t frame_index) const
    {
        (void)frame_index;
    }
};
#endif // QUICKGUI_USE_FFMPEG



#ifdef QUICKGUI_USE_CV
struct ReaderCVCap
{
    VideoSource      m_src = {};
    cv::VideoCapture m_cvcap = {};
    int              m_cvformat = {};
    uint32_t         m_width = {};
    uint32_t         m_height = {};
    uint32_t         m_nframes = {};
    uint32_t         m_curr_loop = {};
    float            m_fps = {};
    fmsecs           m_dt = {};
    char             m_fourcc[4] = {};

    void getprops()
    {
        m_cvformat = (int)m_cvcap.get(cv::CAP_PROP_FORMAT);
        m_width = (uint32_t)m_cvcap.get(cv::CAP_PROP_FRAME_WIDTH);
        m_height = (uint32_t)m_cvcap.get(cv::CAP_PROP_FRAME_HEIGHT);
        m_nframes = (m_src.source_type == VideoSource::FILE) ?
            (uint32_t)m_cvcap.get(cv::CAP_PROP_FRAME_COUNT) : 0u;
        m_fps = (float)m_cvcap.get(cv::CAP_PROP_FPS);
        m_dt = fmsecs(1000.f / m_fps);
        const uint32_t fourcc = static_cast<uint32_t>(m_cvcap.get(cv::CAP_PROP_FOURCC));
        m_fourcc[0] = (char)((fourcc & 0x000000ffu) >>  0);
        m_fourcc[1] = (char)((fourcc & 0x0000ff00u) >>  8);
        m_fourcc[2] = (char)((fourcc & 0x00ff0000u) >> 16);
        m_fourcc[3] = (char)((fourcc & 0xff000000u) >> 24);
        printf("fourcc=%lf %u -> '%c'.'%c'.'%c'.'%c'\n", m_cvcap.get(cv::CAP_PROP_FOURCC), fourcc, m_fourcc[0], m_fourcc[1], m_fourcc[2], m_fourcc[3]);
        cv::String backend = m_cvcap.getBackendName();
        int cvtype_orig = m_cvformat;
        m_cvformat = cvtype_to_video(m_cvformat);
        QUICKGUI_LOGF(R"(video properties:
    width={}
    height={}
    fourcc={}.{}.{}.{}
    format={} --> {}
    #channels={}
    #frames={}
    fps={}/s
    dt={}ms
    backend={}
)",
                      m_width, m_height, m_fourcc[0], m_fourcc[1], m_fourcc[2], m_fourcc[3],
                      cvtype_str(cvtype_orig), cvtype_str(m_cvformat),
                      cvtype_lookup(m_cvformat).num_channels, m_nframes, m_fps, m_dt.count(),
                      c4::substr(backend.data(), backend.size()));
    }

    void init(VideoSource const& src)
    {
        m_src = src;
        if(src.source_type == VideoSource::FILE)
            init_file_(src.file);
        else if(src.source_type == VideoSource::CAMERA)
            init_cam_(src.camera);
        else
            C4_ERROR("unknown source");
    }

    void init_file_(VideoSource::VideoSourceFile const& src)
    {
        C4_CHECK_MSG(c4::fs::file_exists(src.filename.c_str()), "video file does not exist: %s", src.filename.c_str());
        m_cvcap.open(cv::String(src.filename), CV_CAP_V4L2);
        C4_CHECK_MSG(m_cvcap.isOpened(), "failed to open video file: %s", src.filename.c_str());
        QUICKGUI_LOGF(R"(opened video: {})", src.filename);
        getprops();
    }

    void init_cam_(VideoSource::VideoSourceCamera const& src)
    {
        m_cvcap.open(src.index);
        C4_CHECK_MSG(m_cvcap.isOpened(), "failed to open camera[%d]", src.index);
        QUICKGUI_LOGF(R"(opened camera: {})", src.index);
        getprops();
        // https://stackoverflow.com/questions/16092802/capturing-1080p-at-30fps-from-logitech-c920-with-opencv-2-4-3
        bool src_overrides = false;
        if(src.codec[0])
        {
            src_overrides = true;
            c4::csubstr c = {src.codec, 4};
            QUICKGUI_LOGF("setting camera codec: {}", c);
            //C4_CHECK(m_cvcap.set(CV_CAP_PROP_FOURCC, CV_FOURCC('M', 'J', 'P', 'G')));
            m_cvcap.set(CV_CAP_PROP_FOURCC, CV_FOURCC(c[0], c[1], c[2], c[3]));
        }
        if(src.width)
        {
            src_overrides = true;
            QUICKGUI_LOGF("setting camera width: {}", src.width);
            //C4_CHECK(m_cvcap.set(CV_CAP_PROP_FRAME_WIDTH, src.width));
            m_cvcap.set(CV_CAP_PROP_FRAME_WIDTH, src.width);
        }
        if(src.height)
        {
            src_overrides = true;
            QUICKGUI_LOGF("setting camera height: {}", src.height);
            //C4_CHECK(m_cvcap.set(CV_CAP_PROP_FRAME_HEIGHT, src.height));
            m_cvcap.set(CV_CAP_PROP_FRAME_HEIGHT, src.height);
        }
        if(src_overrides)
        {
            getprops();
        }
    }

    size_t frame_bytes() const
    {
        return cvtype_bytes(m_cvformat) * m_width * m_height;
    }

    imgviewtype::data_type_e data_type() const
    {
        return cvtype_lookup(m_cvformat).data_type;
    }

    uint32_t num_channels() const
    {
        return cvtype_lookup(m_cvformat).num_channels;
    }

    bool finished() const
    {
        if(m_src.file.loop)
            return false;
        return (uint32_t)m_cvcap.get(cv::CAP_PROP_POS_FRAMES) == (uint32_t)m_cvcap.get(cv::CAP_PROP_FRAME_COUNT);
    }

    std::chrono::nanoseconds time() const
    {
        using T = std::chrono::nanoseconds::rep;
        return std::chrono::nanoseconds((T)(1.e6 * m_cvcap.get(cv::CAP_PROP_POS_MSEC)));
    }

    void time(std::chrono::nanoseconds t)
    {
        m_cvcap.set(cv::CAP_PROP_POS_MSEC, 1.e-6 * (double)t.count());
    }

    uint32_t frame() const
    {
        return (uint32_t)m_cvcap.get(cv::CAP_PROP_POS_FRAMES);
    }

    void frame(uint32_t frame_index)
    {
        m_cvcap.set(cv::CAP_PROP_POS_FRAMES, (double)frame_index);
    }

    bool frame_grab()
    {
        bool gotit = m_cvcap.grab();
        if(!gotit)
        {
            if(m_src.source_type == VideoSource::FILE && m_src.file.loop)
            {
                ++m_curr_loop;
                QUICKGUI_LOGF("looping video: #frames={} #loops={}x", m_nframes, m_curr_loop);
                C4_CHECK(m_cvcap.set(cv::CAP_PROP_POS_FRAMES, 0));
                gotit = m_cvcap.grab();
            }
        }
        else
        return gotit;
    }

    bool frame_read(wimgview *v)
    {
        cv::Size matsize(/*cols*/(int)m_width, /*rows*/(int)m_height);
        cv::Mat matview(matsize, m_cvformat, v->buf);
        // ensure that the cvmat is pointing at our data
        auto check_matview = [&]{
            if(!same_mat(matview, *v))
            {
                QUICKGUI_LOGF("elmsize={} #elms={} view={}({}) pimpl={}({})",
                              matview.elemSize(), matview.total(),
                              cvtype_str(matview.type()), matview.type(),
                              cvtype_str(m_cvformat), m_cvformat);
                C4_ERROR("not same mat");
            }
        };
        check_matview();
        m_cvcap.retrieve(matview);
        check_matview();
        return true;
    }

};
#endif


struct ReaderImages
{
    using FileEntries  = c4::fs::EntryList;
    using FileEntry    = c4::fs::EntryList::const_iterator;

    VideoSource::VideoSourceImages m_src = {};
    uint32_t           m_width = {};
    uint32_t           m_height = {};
    uint32_t           m_nframes = {};
    uint32_t           m_curr_loop = {};
    float              m_fps = {};
    fmsecs             m_dt = {};
    std::vector<char>  m_data = {};
    imgview            m_firstview = {};
    FileEntries        m_filenames = {};
    FileEntry          m_filenames_curr = {};
    FileEntry          m_filenames_next = {};
    std::vector<char*> m_filenames_names = {};
    std::vector<char > m_filenames_arena = {};
    std::vector<char > m_filenames_scratch = {};

    void init(VideoSource::VideoSourceImages const& src)
    {
        m_src = src;
        C4_CHECK_MSG(c4::fs::dir_exists(src.directory.c_str()), "directory=%s", src.directory.c_str());
        if(m_filenames_names.size() < 1024)
            m_filenames_names.resize(1024);
        if(m_filenames_arena.size() < 256)
            m_filenames_arena.resize(256);
        if(m_filenames_scratch.size() < 256)
            m_filenames_scratch.resize(256);
        m_filenames = {
            m_filenames_arena.data(), m_filenames_arena.size(),
            m_filenames_names.data(), m_filenames_names.size()};
        c4::fs::maybe_buf<char> scratch = {
            m_filenames_scratch.data(), m_filenames_scratch.size()
        };
        if(!c4::fs::list_entries(src.directory.c_str(), &m_filenames, &scratch))
        {
            m_filenames_arena.resize(m_filenames.arena.required_size);
            m_filenames_names.resize(m_filenames.names.required_size);
            m_filenames_scratch.resize(scratch.required_size);
            m_filenames = {
                m_filenames_arena.data(), m_filenames_arena.size(),
                m_filenames_names.data(), m_filenames_names.size()
            };
            scratch = {m_filenames_scratch.data(), m_filenames_scratch.size()};
            C4_CHECK(c4::fs::list_entries(src.directory.c_str(), &m_filenames, &scratch));
        }
        m_filenames.sort();
        filter_filenames_for_bmp();
        m_nframes = 0;
        for(const char *filename : m_filenames)
        {
            C4_UNUSED(filename);
            //QUICKGUI_LOGF("found {}", c4::to_csubstr(filename));
            C4_ASSERT(c4::fs::file_exists(filename));
            m_nframes++;
        }
        QUICKGUI_LOGF("found {} frames in {}", m_nframes, c4::to_csubstr(src.directory.c_str()));
        reset_frames();
        c4::fs::file_get_contents(*m_filenames_next, &m_data);
        m_firstview = load_bmp(m_data.data(), (uint32_t)m_data.size());
        m_width = (uint32_t)m_firstview.width;
        m_height = (uint32_t)m_firstview.height;
        QUICKGUI_LOGF("video frames: {}x{}px #ch={} data_size={}B", m_width, m_height, m_firstview.num_channels, m_firstview.bytes_required());
        m_curr_loop = 0;
    }

    void filter_filenames_for_bmp()
    {
        auto &ffn = m_filenames;
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

    bool frame_grab()
    {
        if(next_frame())
        {
            return true;
        }
        else
        {
            if(m_src.loop)
            {
                ++m_curr_loop;
                reset_frames();
                next_frame();
                return true;
            }
        }
        return false;
    }

    bool frame_read(wimgview *v)
    {
        C4_ASSERT(m_filenames_curr != ((c4::fs::EntryList const&)m_filenames).end());
        c4::fs::file_get_contents(*m_filenames_curr, &m_data);
        *v = load_bmp(m_data.data(), (uint32_t)m_data.size());
        C4_CHECK(v->width == m_width);
        C4_CHECK(v->height == m_height);
        C4_CHECK(v->num_channels == m_firstview.num_channels);
        return true;
    }

    bool next_frame()
    {
        if(m_filenames_next == ((c4::fs::EntryList const&)m_filenames).end())
            return false;
        m_filenames_curr = m_filenames_next;
        ++m_filenames_next;
        return m_filenames_next != ((c4::fs::EntryList const&)m_filenames).end();
    }

    void reset_frames()
    {
        frame(0);
    }

    void frame(uint32_t frame_index)
    {
        m_filenames_next = ((c4::fs::EntryList const&)m_filenames).begin() + frame_index + 1;
        m_filenames_curr = m_filenames_next - 1;
    }

    uint32_t frame() const
    {
        C4_ASSERT(m_filenames_curr >= m_filenames.begin());
        return (uint32_t)(m_filenames_curr - m_filenames.begin());
    }

    std::chrono::nanoseconds time() const
    {
        using T = std::chrono::nanoseconds::rep;
        const double frame_index = (double)(m_filenames_curr - m_filenames.begin());
        const double secs_sofar = frame_index / (double)m_fps;
        return std::chrono::nanoseconds((T)(1.e9 * secs_sofar));
    }

    /* set time */
    void time(std::chrono::nanoseconds t)
    {
        frame((uint32_t)((0.000001 + (double)m_src.fps * quickgui::dsecs(t).count())));
    }

    bool finished() const
    {
        if(m_src.loop)
            return false;
        return m_filenames_next == m_filenames.end();
    }

    size_t frame_bytes() const
    {
        return m_firstview.bytes_required();
    }

    imgviewtype::data_type_e data_type() const
    {
        return m_firstview.data_type;
    }

    uint32_t num_channels() const
    {
        return m_firstview.num_channels;
    }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct VideoReader::Impl
{
    VideoSource m_src;
    uint32_t    m_width;
    uint32_t    m_height;
    uint32_t    m_nframes;
    float       m_fps;
    fmsecs      m_dt;

    ReaderImages m_images;
    #ifdef QUICKGUI_USE_FFMPEG
    ReaderAVCam m_av_cam;
    ReaderAVFile m_av_video;
    #elif defined(QUICKGUI_USE_CV)
    ReaderCVCap m_cv_cap;
    #endif

    Impl(VideoSource const& src)
        : m_src(src)
        , m_width()
        , m_height()
        , m_nframes()
        , m_fps()
        , m_dt()
        , m_images()
        #ifdef QUICKGUI_USE_FFMPEG
        , m_av_cam()
        , m_av_video()
        #elif defined(QUICKGUI_USE_CV)
        , m_cv_cap()
        #endif
    {
        switch(src.source_type)
        {
        case VideoSource::IMAGES:
            m_images.init(src.images);
            m_width = m_images.m_width;
            m_height = m_images.m_height;
            m_nframes = m_images.m_nframes;
            m_fps = m_images.m_fps;
            m_dt = m_images.m_dt;
            break;
        case VideoSource::FILE:
        {
            #ifdef QUICKGUI_USE_FFMPEG
            m_av_video.init(src.file);
            m_width = (uint32_t)m_av_video.m_width;
            m_height = (uint32_t)m_av_video.m_height;
            m_nframes = m_av_video.m_nframes;
            m_fps = m_images.m_fps;
            m_dt = m_images.m_dt;
            #elif defined(QUICKGUI_USE_CV)
            m_cv_cap.init(src);
            m_width = m_cv_cap.m_width;
            m_height = m_cv_cap.m_height;
            m_nframes = m_cv_cap.m_nframes;
            m_fps = m_cv_cap.m_fps;
            m_dt = m_cv_cap.m_dt;
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        }
        case VideoSource::CAMERA:
        {
            #ifdef QUICKGUI_USE_FFMPEG
            m_av_cam.init(src.camera);
            m_width = (uint32_t)m_av_cam.m_width;
            m_height = (uint32_t)m_av_cam.m_height;
            m_nframes = 0u;
            m_fps = m_av_cam.m_fps;
            m_dt = m_av_cam.m_dt;
            #elif defined(QUICKGUI_USE_CV)
            m_cv_cap.init(src);
            m_width = m_cv_cap.m_width;
            m_height = m_cv_cap.m_height;
            m_nframes = 0u;
            m_fps = m_cv_cap.m_fps;
            m_dt = m_cv_cap.m_dt;
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        }
        default:
            C4_NOT_IMPLEMENTED();
        }
    }

    bool frame_grab()
    {
        switch(m_src.source_type)
        {
        case VideoSource::CAMERA:
            #ifdef QUICKGUI_USE_CV
            return m_cv_cap.frame_grab();
            #elif defined(QUICKGUI_USE_FFMPEG)
            return m_av_cam.frame_grab();
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        case VideoSource::FILE:
        {
            #ifdef QUICKGUI_USE_CV
            return m_cv_cap.frame_grab();
            #elif defined(QUICKGUI_USE_FFMPEG)
            return m_av_video.frame_grab();
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        }
        case VideoSource::IMAGES:
            return m_images.frame_grab();
        default:
            C4_NOT_IMPLEMENTED();
        }
        return false;
    }

    bool frame_read(wimgview *v)
    {
        // exit early if the data cannot accomodate the frame
        if(v->bytes_required() < frame_bytes())
            return false;
        switch(m_src.source_type)
        {
        case VideoSource::CAMERA:
            #ifdef QUICKGUI_USE_CV
            return m_cv_cap.frame_read(v);
            #elif defined(QUICKGUI_USE_FFMPEG)
            return m_av_cam.frame_read(v);
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        case VideoSource::FILE:
        {
            #ifdef QUICKGUI_USE_CV
            return m_cv_cap.frame_read(v);
            #elif defined(QUICKGUI_USE_FFMPEG)
            return m_av_video.frame_read(v);
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        }
        case VideoSource::IMAGES:
            return m_images.frame_read(v);
        default:
            C4_NOT_IMPLEMENTED();
        }
        return false;
    }

    uint32_t frame() const
    {
        switch(m_src.source_type)
        {
        case VideoSource::CAMERA:
            break;
        case VideoSource::IMAGES:
            return m_images.frame();
        case VideoSource::FILE:
            #ifdef QUICKGUI_USE_CV
            return m_cv_cap.frame();
            #elif defined(QUICKGUI_USE_FFMPEG)
            return m_av_video.frame();
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

    void frame(uint32_t frame_index)
    {
        QUICKGUI_LOGF("reset frame: {}", frame_index);
        switch(m_src.source_type)
        {
        case VideoSource::IMAGES:
            m_images.frame(frame_index);
            break;
        case VideoSource::CAMERA:
            QUICKGUI_LOGF("cannot set frame on camera");
            break;
        case VideoSource::FILE:
            #ifdef QUICKGUI_USE_CV
            m_cv_cap.frame(frame_index);
            #elif defined(QUICKGUI_USE_FFMPEG)
            m_av_video.frame(frame_index);
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
        switch(m_src.source_type)
        {
        case VideoSource::IMAGES:
            return m_images.time();
        case VideoSource::CAMERA:
            break;
        case VideoSource::FILE:
            #ifdef QUICKGUI_USE_CV
            return m_cv_cap.time();
            #elif defined(QUICKGUI_USE_FFMPEG)
            return m_av_video.time();
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        default:
            C4_NOT_IMPLEMENTED();
        }
        return std::chrono::nanoseconds::zero();
    }

    /* set time */
    void time(std::chrono::nanoseconds t)
    {
        switch(m_src.source_type)
        {
        case VideoSource::IMAGES:
            m_images.time(t);
            break;
        case VideoSource::CAMERA:
            QUICKGUI_LOGF("cannot set time on camera");
            break;
        case VideoSource::FILE:
            #ifdef QUICKGUI_USE_CV
            m_cv_cap.time(t);
            #elif defined(QUICKGUI_USE_FFMPEG)
            m_av_video.time(t);
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        default:
            C4_NOT_IMPLEMENTED();
            break;
        }
    }

    bool finished() const
    {
        switch(m_src.source_type)
        {
        case VideoSource::IMAGES:
            return m_images.finished();
        case VideoSource::CAMERA:
            return false;
        case VideoSource::FILE:
            #ifdef QUICKGUI_USE_CV
            return m_cv_cap.finished();
            #elif defined(QUICKGUI_USE_FFMPEG)
            return m_av_video.finished();
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        default:
            C4_NOT_IMPLEMENTED();
        }
        return false;
    }

    size_t frame_bytes() const
    {
        switch(m_src.source_type)
        {
        case VideoSource::IMAGES:
            return m_images.frame_bytes();
        case VideoSource::CAMERA:
            #ifdef QUICKGUI_USE_CV
            return m_cv_cap.frame_bytes();
            #elif defined(QUICKGUI_USE_FFMPEG)
            return m_av_cam.frame_bytes();
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        case VideoSource::FILE:
            #ifdef QUICKGUI_USE_CV
            return m_cv_cap.frame_bytes();
            #elif defined(QUICKGUI_USE_FFMPEG)
            return m_av_video.frame_bytes();
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        default:
            C4_NOT_IMPLEMENTED();
        }
        return 0;
    }

    imgviewtype::data_type_e data_type() const
    {
        switch(m_src.source_type)
        {
        case VideoSource::IMAGES:
            return m_images.data_type();
        case VideoSource::CAMERA:
            #ifdef QUICKGUI_USE_CV
            return m_cv_cap.data_type();
            #elif defined(QUICKGUI_USE_FFMPEG)
            return m_av_cam.data_type();
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        case VideoSource::FILE:
            #ifdef QUICKGUI_USE_CV
            return m_cv_cap.data_type();
            #elif defined(QUICKGUI_USE_FFMPEG)
            return m_av_video.data_type();
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        default:
            C4_NOT_IMPLEMENTED();
        }
        return {};
    }

    uint32_t num_channels() const
    {
        switch(m_src.source_type)
        {
        case VideoSource::IMAGES:
            return m_images.num_channels();
        case VideoSource::CAMERA:
            #ifdef QUICKGUI_USE_CV
            return m_cv_cap.num_channels();
            #elif defined(QUICKGUI_USE_FFMPEG)
            return m_av_cam.num_channels();
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        case VideoSource::FILE:
            #ifdef QUICKGUI_USE_CV
            return m_cv_cap.num_channels();
            #elif defined(QUICKGUI_USE_FFMPEG)
            return m_av_video.num_channels();
            #else
            C4_NOT_IMPLEMENTED();
            #endif
            break;
        default:
            C4_NOT_IMPLEMENTED();
        }
        return {};
    }

};


//-----------------------------------------------------------------------------

VideoReader::VideoReader(VideoSource const& src)
    : m_pimpl()
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
    return m_pimpl->frame_bytes();
}

imgview::data_type_e VideoReader::data_type() const
{
    return m_pimpl->data_type();
}

uint32_t VideoReader::num_channels() const
{
    return m_pimpl->num_channels();
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
    return m_pimpl->frame_grab();
}

bool VideoReader::frame_read(wimgview *v)
{
    return m_pimpl->frame_read(v);
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
