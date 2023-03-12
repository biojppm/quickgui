#ifndef QUICKGUI_VIDEO_VIDEO_SOURCE_HPP_
#define QUICKGUI_VIDEO_VIDEO_SOURCE_HPP_

#include <string>

namespace quickgui {

struct VideoSource
{
    struct VideoSourceCamera
    {
        int index = 0;
        int width = 0;
        int height = 0;
        char codec[4] = {};
        bool has_codec() const { return *(uint32_t const*)codec != 0; }
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

#endif /* QUICKGUI_VIDEO_VIDEO_SOURCE_HPP_ */
