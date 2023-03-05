#include "quickgui/stb_image_data.hpp"
#include <c4/error.hpp>
#include <c4/szconv.hpp>

C4_SUPPRESS_WARNING_MSVC_PUSH
C4_SUPPRESS_WARNING_GCC_CLANG_PUSH
C4_SUPPRESS_WARNING_MSVC(4296) // expression is always true
C4_SUPPRESS_WARNING_MSVC(4242) // conversion from 'int' to 'short', possible loss of data
C4_SUPPRESS_WARNING_MSVC(4244) // conversion from 'int' to 'short', possible loss of data
C4_SUPPRESS_WARNING_CLANG("-Wimplicit-int-conversion")
C4_SUPPRESS_WARNING_CLANG("-Wcast-align")
C4_SUPPRESS_WARNING_GCC_CLANG("-Wsign-conversion")
C4_SUPPRESS_WARNING_GCC_CLANG("-Wdouble-promotion")
C4_SUPPRESS_WARNING_GCC("-Wconversion")
C4_SUPPRESS_WARNING_GCC("-Wuseless-cast")
C4_SUPPRESS_WARNING_GCC("-Wtype-limits")
#define STB_IMAGE_IMPLEMENTATION
#include "quickgui/stb_image.h"
C4_SUPPRESS_WARNING_MSVC_POP
C4_SUPPRESS_WARNING_GCC_CLANG_POP


namespace quickgui {

stb_image_data::stb_image_data(quickgui::ccharspan file_contents, RequiredChannels channels)
    : data()
    , width()
    , height()
    , num_channels()
{
    int len = c4::szconv<int>(file_contents.size());
    stbi_uc const *buffer = (stbi_uc const*) file_contents.data();
    int required_channels = channels == RequiredChannels::defer ? 0 : (int)channels;
    int w = 0, h = 0, nch = 0;
    data = (char*)stbi_load_from_memory(buffer, len, &w, &h, &nch, required_channels);
    C4_CHECK_MSG(data, "failed reading image: %s", stbi_failure_reason());
    C4_CHECK(w > 0);
    C4_CHECK(h > 0);
    C4_CHECK(nch > 0);
    C4_CHECK(nch <= 4);
    width = (uint32_t)w;
    height = (uint32_t)h;
    num_channels = (channels == RequiredChannels::defer ? (uint32_t)nch : (uint32_t)channels);
}

stb_image_data::~stb_image_data()
{
    if(data)
        stbi_image_free(data);
}

} // namespace quickgui
