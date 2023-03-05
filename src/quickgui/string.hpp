#ifndef QUICKGUI_STRING_HPP_
#define QUICKGUI_STRING_HPP_

#include <c4/std/string.hpp>
#include <c4/span.hpp>

namespace quickgui {

/// @todo use a restricted string
using String = std::string;

using charspan = c4::span<char>;
using ccharspan = c4::span<const char>;

} // namespace quickgui

#endif /* QUICKGUI_STRING_HPP_ */
