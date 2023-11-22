#ifndef QUICKGUI_IMGVIEW_FWD_HPP_
#define QUICKGUI_IMGVIEW_FWD_HPP_

#include <stdint.h>

namespace quickgui {

struct imgviewtype;
template<class T>
struct basic_imgview;
using wimgview = basic_imgview<uint8_t>;
using imgview = basic_imgview<const uint8_t>;

} // namespace quickgui

#endif // QUICKGUI_IMGVIEW_FWD_HPP_
