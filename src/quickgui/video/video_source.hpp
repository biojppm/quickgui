#ifndef QUICKGUI_VIDEO_VIDEO_SOURCE_HPP_
#define QUICKGUI_VIDEO_VIDEO_SOURCE_HPP_

#include <string>
#include <c4/error.hpp>

C4_SUPPRESS_WARNING_GCC_CLANG_PUSH
C4_SUPPRESS_WARNING_GCC_CLANG("-Wold-style-cast")

namespace quickgui {

struct VideoSource
{
    struct VideoSourceCamera
    {
        #ifdef QUICKGUI_USE_CV
        int index = 0;
        int width = 0;
        int height = 0;
        char codec[4] = {};
        bool has_codec() const { return *(uint32_t const*)codec != 0; }
        #elif defined(QUICKGUI_USE_FFMPEG)
        const char *input_device = "v4l2";
        const char *device = "/dev/video0";
        const char *input_format = "mjpeg"; // "mjpeg", "yuyv422";
        int width = 1280;
        int height = 720;
        int framerate = 30;
        int log_level = 32; // INFO see <avutil/log.h>
        int log_level_init = 56; // TRACE see <avutil/log.h>
        #else
        #error no video input
        #endif
    } camera;
    struct VideoSourceImages
    {
        std::string directory = {};
        bool loop = true;
        float fps = 30.f;
    } images;
    struct VideoSourceFile
    {
        std::string filename = {};
        bool loop = true;
    } file;
    typedef enum : uint8_t { CAMERA, IMAGES, FILE, } SourceType;
    SourceType source_type = FILE;
    bool loop() const
    {
        return (source_type == FILE && file.loop)
            || (source_type == IMAGES && images.loop);
    }
    const char* name() const
    {
        if(source_type == FILE)
            return file.filename.c_str();
        else if(source_type == IMAGES)
            return images.directory.c_str();
        else if(source_type == CAMERA)
            return "camera";
        else
            return "unknown";
    }
};

} // namespace quickgui

C4_SUPPRESS_WARNING_GCC_CLANG_POP

#endif /* QUICKGUI_VIDEO_VIDEO_SOURCE_HPP_ */
